#include "rpc_action.hh"
#include "rpc_broadcast.hh"
#include "rpc_server.hh"
#include "rpc_thread.hh"

#include <hwloc.h>

#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>

#include <atomic>
#include <array>
#include <cmath>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace rpc {

namespace {

// Convert string to integer
int stoi1(const char *str, int dflt) {
  if (str) {
    try {
      const std::string str1(str);
      std::size_t pos;
      int res = std::stoi(str1, &pos);
      if (pos == str1.length())
        return res;
    } catch (...) {
      // do nothing
    }
  }
  return dflt;
}

// Process/node mapping
struct proc_map_t {
  // current node
  int node, nodes;
  // global MPI rank/size
  int rank, size;
  // MPI rank/size for the current node
  int local_rank, local_size;

  // TODO: determine nodes and local rank from hwloc, probably best via
  // MPI_Comm_split
  proc_map_t() {
    // rank = stoi1(getenv("OMPI_COMM_WORLD_RANK"),
    //              stoi1(getenv("MV2_COMM_WORLD_RANK"), 0));
    // size = stoi1(getenv("OMPI_COMM_WORLD_SIZE"),
    //              stoi1(getenv("MV2_COMM_WORLD_SIZE"), server->size()));
    // local_rank = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_RANK"),
    //                    stoi1(getenv("MV2_COMM_WORLD_LOCAL_RANK"), 0));
    // local_size = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_SIZE"),
    //                    stoi1(getenv("MV2_COMM_WORLD_LOCAL_SIZE"), 1));
    // node = rank / local_size; // this assumes a regular layout
    // nodes = size / local_size;
    size = server->size();
    RPC_ASSERT(size > 0);
    nodes = stoi1(getenv("RPC_NODES"), -1);
    RPC_ASSERT(nodes > 0 && size % nodes == 0);
    local_size = size / nodes;
    RPC_ASSERT(local_size > 0 && size % local_size == 0);
    rank = server->rank();
    RPC_ASSERT(rank >= 0 && rank < size);
    node = rank / local_size;
    RPC_ASSERT(node >= 0 && node < nodes);
    local_rank = rank % local_size;
    RPC_ASSERT(local_rank >= 0 && local_rank < local_size);
  }
};

bool output_affinity(std::ostream &os, const hwloc_topology_t &topology,
                     const proc_map_t &proc_map) {
  os << "   "
     << "N" << proc_map.node << " "
     << "L" << proc_map.local_rank << " "
     << "P" << proc_map.rank << " "
     << "T" << this_thread::get_worker_id() << " ";
#ifdef QTHREAD_VERSION
  os << "(S" << qthread_shep() << ") ";
#endif

  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth >= 0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus > 0);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  RPC_ASSERT(cpuset);
  const int ierr = hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (!ierr) {

    hwloc_cpuset_t lcpuset = hwloc_bitmap_alloc();
    RPC_ASSERT(lcpuset);
    for (int pu_num = 0; pu_num < num_pus; ++pu_num) {
      const hwloc_obj_t pu_obj =
          hwloc_get_obj_by_depth(topology, pu_depth, pu_num);
      if (hwloc_bitmap_isset(cpuset, pu_obj->os_index))
        hwloc_bitmap_set(lcpuset, pu_num);
    }
    char lcpuset_buf[1000];
    hwloc_bitmap_list_snprintf(lcpuset_buf, sizeof lcpuset_buf, lcpuset);
    hwloc_bitmap_free(lcpuset);
    char cpuset_buf[1000];
    hwloc_bitmap_list_snprintf(cpuset_buf, sizeof cpuset_buf, cpuset);

    os << "PU set L#{" << lcpuset_buf << "} "
       << "P#{" << cpuset_buf << "}\n";

  } else {
    os << "[could not determine CPU bindings]\n";
  }

  hwloc_bitmap_free(cpuset);
  return ierr;
}

bool set_affinity(std::ostream &os, const hwloc_topology_t &topology,
                  const proc_map_t &proc_map) {
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth >= 0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus > 0);

  const int thread = this_thread::get_worker_id();
  const int num_threads = thread::hardware_concurrency();
  const int node_num_threads = proc_map.local_size * num_threads;
  const int node_thread = proc_map.local_rank * num_threads + thread;

  // const bool oversubscribing = node_num_threads > num_pus;
  const bool undersubscribing = node_num_threads < num_pus;

  const int thread_num_pus = undersubscribing ? num_pus / node_num_threads : 1;
  const int thread_pu_num = node_thread * num_pus / node_num_threads;
  RPC_ASSERT(thread_pu_num + thread_num_pus <= num_pus);

  // Note: Even when undersubscribing we are binding the thread to a
  // single PU
  const hwloc_obj_t pu_obj =
      hwloc_get_obj_by_depth(topology, pu_depth, thread_pu_num);
  RPC_ASSERT(pu_obj);
  // hwloc_cpuset_t cpuset = pu_obj->cpuset;
  const hwloc_cpuset_t cpuset = hwloc_bitmap_dup(pu_obj->cpuset);
  int ierr;
  ierr = hwloc_set_cpubind(topology, cpuset,
                           HWLOC_CPUBIND_THREAD | HWLOC_CPUBIND_STRICT);
  if (ierr)
    ierr = hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);

  hwloc_bitmap_free(cpuset);
  return ierr;
}

bool run(bool do_set, bool do_output, const proc_map_t *proc_map_,
         const hwloc_topology_t *topology_, std::atomic<bool> *worker_done,
         std::vector<std::string> *infos_) {
  const hwloc_topology_t &topology = *topology_;
  const proc_map_t &proc_map = *proc_map_;
  std::vector<std::string> &infos = *infos_;

  const int thread = this_thread::get_worker_id();
  const int nthreads = thread::hardware_concurrency();
  assert(thread >= 0 && thread < nthreads);

  bool have_error = false;
  if (!worker_done[thread]) {
    worker_done[thread] = true;
    std::stringstream os;
    if (do_set)
      have_error |= set_affinity(os, topology, proc_map);
    if (do_output)
      have_error |= output_affinity(os, topology, proc_map);
    infos[thread] = os.str();
  }

  bool all_done = true;
  for (int t = 0; t < nthreads; ++t)
    all_done &= worker_done[t];
  if (!all_done) {
    // block for some time to keep the current core busy
    double x = 0.1;
    for (std::ptrdiff_t i = 0; i < 1000 * 1000; ++i)
      x = std::sqrt(x);
    volatile double r __attribute__((unused)) = x;
  }

  return have_error;
}

std::string run_on_threads(bool do_set, bool do_output,
                           const proc_map_t &proc_map,
                           const hwloc_topology_t &topology) {
  std::stringstream os;
  int nthreads = thread::hardware_concurrency();
  int nsubmit = 100 * nthreads;
  int nattempts = 10;
  for (int attempt = 0; attempt < nattempts; ++attempt) {
    std::atomic<bool> worker_done[nthreads];
    for (int thread = 0; thread < nthreads; ++thread)
      worker_done[thread] = false;
    std::vector<std::string> infos(nthreads);
    std::vector<future<bool> > fs;
    for (int submit = 0; submit < nsubmit; ++submit)
      fs.push_back(async(run, do_set, do_output, &proc_map, &topology,
                         &worker_done[0], &infos));
    // Prod the scheduler, as per advice from Dylan Stark
    // <dstark@sandia.gov> 2014-08-07
    this_thread::yield();
    bool have_all_infos = true;
    for (auto &f : fs)
      have_all_infos &= !f.get();
    if (have_all_infos) {
      for (const auto &info : infos)
        os << info;
      return os.str();
    }
  }
  os << "P" << server->rank() << ": WARNING: Could not set CPU bindings\n";
  return os.str();
}
}

namespace {
// Saved messages for later output
const int num_messages = 3;
const char *descriptions[num_messages] = {
  "Process maps:\n", "Original CPU bindings:\n", "New CPU bindings:\n",
};
std::array<std::string, num_messages> messages;
}

// This routine is called early, and runs on each process
void set_cpu_bindings() {
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  const proc_map_t proc_map;
  const bool do_set = true;
  const bool do_output = proc_map.node == 0;

  // for (int proc = 0; proc < proc_map.size; ++proc) {
  //   char name[MPI_MAX_PROCESSOR_NAME + 1];
  //   int namelen;
  //   MPI_Get_processor_name(name, &namelen);
  //   name[namelen] = '\0';
  //   std::cout << "P" << proc << ": " << name << "\n";
  // }

  // for (int proc = 0; proc < proc_map.size; ++proc) {
  //   const int machine_depth =
  //       hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_MACHINE);
  //   RPC_ASSERT(machine_depth >= 0);
  //   const int num_machines = hwloc_get_nbobjs_by_depth(topology,
  // machine_depth);
  //   RPC_ASSERT(num_machines > 0);
  //   const hwloc_obj_t machine_obj =
  //       hwloc_get_obj_by_depth(topology, machine_depth, 0);
  //   RPC_ASSERT(machine_obj);
  //   const char *hostname = hwloc_obj_get_info_by_name(machine_obj,
  // "HostName");
  //   std::cout << "P" << proc << ": " << hostname << "\n";
  // }

  if (do_output && proc_map.node == 0) {
    std::stringstream os;
    os << "   "
       << "N" << proc_map.node << "/" << proc_map.nodes << " "
       << "L" << proc_map.local_rank << "/" << proc_map.local_size << " "
       << "P" << proc_map.rank << "/" << proc_map.size << "\n";
    messages[0] = os.str();
  }

  if (do_output && proc_map.node == 0) {
    messages[1] = run_on_threads(false, true, proc_map, topology);
  }

  if (do_set)
    run_on_threads(true, false, proc_map, topology);

  if (do_set && do_output && proc_map.node == 0) {
    messages[2] = run_on_threads(false, true, proc_map, topology);
  }

  hwloc_topology_destroy(topology);
}

auto hwloc_get_messages() -> std::array<std::string, num_messages> {
  return messages;
}
RPC_DECLARE_ACTION(hwloc_get_messages);

// This routine is called after setup, and runs only on one process
void output_cpu_bindings() {
  auto foutputs =
      rpc::broadcast(rpc::find_all_processes(), hwloc_get_messages_action());
  std::vector<std::array<std::string, num_messages> > outputs;
  for (auto &fstr : foutputs)
    outputs.push_back(fstr.get());
  for (int stage = 0; stage < num_messages; ++stage) {
    std::cout << descriptions[stage];
    for (const auto &output : outputs)
      std::cout << output[stage];
  }
}
}

RPC_IMPLEMENT_ACTION(rpc::hwloc_get_messages);
