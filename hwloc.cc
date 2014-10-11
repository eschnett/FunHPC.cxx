#include "hwloc.hh"

#include "rpc.hh"

#include <hwloc.h>

#include <cereal/access.hpp>
#include <cereal/types/string.hpp>

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::atomic;
using std::cerr;
using std::cout;
using std::flush;
using std::getenv;
using std::ostream;
using std::ostringstream;
using std::stoi;
using std::string;
using std::vector;

namespace {

int stoi1(const char *str, int dflt) {
  if (str) {
    try {
      const string str1(str);
      size_t pos;
      int res = stoi(str1, &pos);
      if (pos == str1.length())
        return res;
    } catch (...) {
      // do nothing
    }
  }
  return dflt;
}

bool output_affinity(ostream &os, const hwloc_topology_t &topology) {
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth >= 0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus > 0);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  RPC_ASSERT(cpuset);
  const int ierr = hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr) {
    hwloc_bitmap_free(cpuset);
    os << "   "
       << "P" << rpc::server->rank() << " "
       << "T" << rpc::this_thread::get_worker_id() << " "
#ifdef QTHREAD_VERSION
       << "(S" << qthread_shep() << ") "
#endif
       << "[could not determine CPU bindings]\n";
    return true;
  }
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
  hwloc_bitmap_free(cpuset);

  os << "   "
     << "P" << rpc::server->rank() << " "
     << "T" << rpc::this_thread::get_worker_id() << " "
#ifdef QTHREAD_VERSION
     << "(S" << qthread_shep() << ") "
#endif
     << "PU set L#{" << lcpuset_buf << "} "
     << "P#{" << cpuset_buf << "}\n";
  return false;
}

bool set_affinity(ostream &os, const hwloc_topology_t &topology) {
  const int rank =
      stoi1(getenv("OMPI_COMM_WORLD_RANK"),
            stoi1(getenv("MV2_COMM_WORLD_RANK"), rpc::server->rank()));
  const int size =
      stoi1(getenv("OMPI_COMM_WORLD_SIZE"),
            stoi1(getenv("MV2_COMM_WORLD_SIZE"), rpc::server->size()));
  const int local_rank = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_RANK"),
                               stoi1(getenv("MV2_COMM_WORLD_LOCAL_RANK"), 0));
  const int local_size = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_SIZE"),
                               stoi1(getenv("MV2_COMM_WORLD_LOCAL_SIZE"), 1));
  RPC_ASSERT(rank >= 0 && rank < size);
  RPC_ASSERT(local_rank >= 0 && local_rank < local_size);
  RPC_ASSERT(rank >= local_rank && local_size <= size);

  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth >= 0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus > 0);

  const int thread = rpc::this_thread::get_worker_id();
  const int num_threads = rpc::thread::hardware_concurrency();
  const int node_num_threads = local_size * num_threads;
  const int node_thread = local_rank * num_threads + thread;

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
  if (ierr)
    os << "Could not set CPU binding for thread " << thread << "\n";
  hwloc_bitmap_free(cpuset);
  return ierr;
}
}

struct hwloc_result_t {
  bool error_set;
  bool error_output;
  string message;

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(error_set, error_output, message);
  }
};

void hwloc_run(bool do_set, bool do_output, int nthreads,
               atomic<bool> *worker_done, vector<hwloc_result_t> *infos_) {
  vector<hwloc_result_t> &infos = *infos_;
  ostringstream os;
  const int thread = rpc::this_thread::get_worker_id();
  assert(thread >= 0 && thread < nthreads);
  bool error_set = false;
  bool error_output = false;
  if (!worker_done[thread]) {
    worker_done[thread] = true;
    // TODO: This leaks topologies???
    hwloc_topology_t topology;
    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    if (do_set)
      error_set = set_affinity(os, topology);
    if (do_output)
      error_output = output_affinity(os, topology);
    infos[thread] = { error_set, error_output, os.str() };
  }
  bool all_done = false;
  for (int t = 0; t < nthreads; ++t)
    all_done |= worker_done[t];
  if (!all_done) {
    // block for some time to keep the current core busy
    double x = 0.1;
    for (ptrdiff_t i = 0; i < 1000 * 1000; ++i)
      x = sqrt(x);
    volatile double r __attribute__((unused)) = x;
  }
}

string hwloc_run_on_threads(bool do_set, bool do_output) {
  ostringstream os;
  int nthreads = rpc::thread::hardware_concurrency();
  int nsubmit = 100 * nthreads;
  int nattempts = 10;
  for (int attempt = 1; attempt <= nattempts; ++attempt) {
    // os << "Attempt #" << attempt << "\n";
    vector<rpc::future<void> > fs;
    atomic<bool> worker_done[nthreads];
    for (int thread = 0; thread < nthreads; ++thread)
      worker_done[thread] = false;
    vector<hwloc_result_t> infos(nthreads);
    for (int submit = 0; submit < nsubmit; ++submit)
      fs.push_back(rpc::async(hwloc_run, do_set, do_output, nthreads,
                              &worker_done[0], &infos));
    for (auto &f : fs)
      f.wait();
    bool have_all_infos = true;
    for (const auto &info : infos)
      have_all_infos &= !(info.error_set || info.error_output);
    if (have_all_infos) {
      for (const auto &info : infos)
        os << info.message;
      return os.str();
    }
  }
  // RPC_ASSERT(0);
  os << "P" << rpc::server->rank() << ": failed\n";
  return os.str();
}
RPC_ACTION(hwloc_run_on_threads);

void hwloc_bindings(bool do_set, bool do_output) {
  auto fs = rpc::broadcast(rpc::find_all_processes(),
                           hwloc_run_on_threads_action(), do_set, do_output);
  for (auto &f : fs)
    cout << f.get();
}
