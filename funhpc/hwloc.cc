#include <cxx/cstdlib.hpp>
#include <funhpc/rexec.hpp>
#include <qthread/future.hpp>
#include <qthread/thread.hpp>

#include <cereal/types/string.hpp>
#include <hwloc.h>
#include <mpi.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace funhpc {

namespace {
int envtoi(const char *var) {
  assert(var);
  char *str = std::getenv(var);
  if (!str) {
    std::cerr << "FunHPC: Could not getenv(\"" << var << "\")\n";
    std::exit(EXIT_FAILURE);
  }
  char *str_end;
  auto res = std::strtol(str, &str_end, 10);
  if (*str_end != '\0') {
    std::cerr << "FunHPC: Could not strol(getenv(\"" << var << "\")=\"" << str
              << "\")\n";
    std::exit(EXIT_FAILURE);
  }
  return res;
}
}

namespace hwloc {

// Thread layout (processes, threads)
struct thread_layout {
  int proc, nprocs;               // MPI processes
  int node, nnodes;               // MPI process layout
  int proc_thread, proc_nthreads; // Threads
  int node_proc, node_nprocs;
  int node_thread, node_nthreads;

  bool invariant() const {
    return (proc >= 0 && proc < nprocs) && (node >= 0 && node < nnodes) &&
           (proc_thread >= 0 && proc_thread < proc_nthreads) &&
           (node_proc >= 0 && node_proc < node_nprocs) &&
           (node_thread >= 0 && node_thread < node_nthreads);
  }

  thread_layout() {
    // TODO: This requires OpenMPI
    proc = envtoi("OMPI_COMM_WORLD_RANK");
    assert(proc == rank());
    nprocs = envtoi("OMPI_COMM_WORLD_SIZE");
    assert(nprocs == size());

    node_proc = envtoi("OMPI_COMM_WORLD_LOCAL_RANK");
    node_nprocs = envtoi("OMPI_COMM_WORLD_LOCAL_SIZE");
    node = cxx::div_floor(proc, node_nprocs).quot;
    nnodes = cxx::div_exact(nprocs, node_nprocs).quot;

    proc_thread = qthread::this_thread::get_worker_id();
    proc_nthreads = qthread::thread::hardware_concurrency();
    node_thread = node_proc * proc_nthreads + proc_thread;
    node_nthreads = node_nprocs * proc_nthreads;

    assert(invariant());
  }
};

// Thread affinity
struct thread_affinity {
  int node_npus;
  bool undersubscribing, oversubscribing;
  int thread_pu, thread_npus;

  bool invariant() const {
    return (node_npus > 0) && (thread_pu >= 0 && thread_npus > 0 &&
                               thread_pu + thread_npus <= node_npus);
  }

  thread_affinity(const hwloc_topology_t topology, const thread_layout &tl) {
    const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
    assert(pu_depth >= 0);

    node_npus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
    assert(node_npus > 0);

    undersubscribing = tl.node_nthreads < node_npus;
    oversubscribing = tl.node_nthreads > node_npus;

    thread_npus =
        oversubscribing ? 1 : cxx::div_exact(node_npus, tl.node_nthreads).quot;
    thread_pu =
        oversubscribing
            ? cxx::div_floor(tl.node_thread * node_npus, tl.node_nthreads).quot
            : cxx::div_exact(tl.node_thread * node_npus, tl.node_nthreads).quot;
    assert(thread_pu + thread_npus <= node_npus);

    assert(invariant());
  }
};

std::string set_affinity(const hwloc_topology_t topology,
                         const thread_affinity &ta) {
  // Note: Even when undersubscribing we are binding the thread to a single PU
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  assert(pu_depth >= 0);
  const hwloc_obj_t pu_obj =
      hwloc_get_obj_by_depth(topology, pu_depth, ta.thread_pu);
  assert(pu_obj);
  const hwloc_cpuset_t cpuset = hwloc_bitmap_dup(pu_obj->cpuset);

  int ierr = hwloc_set_cpubind(topology, cpuset,
                               HWLOC_CPUBIND_THREAD | HWLOC_CPUBIND_STRICT);
  if (ierr)
    ierr = hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr)
    return {" [cannot set CPU bindings]"};

  hwloc_bitmap_free(cpuset);
  return {};
}

std::string get_affinity(const hwloc_topology_t topology) {
  const hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  assert(cpuset);
  int ierr = hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr) {
    hwloc_bitmap_free(cpuset);
    return {" [cannot determine CPU bindings]"};
  }

  const hwloc_cpuset_t lcpuset = hwloc_bitmap_alloc();
  assert(lcpuset);
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  assert(pu_depth >= 0);
  const int npus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  assert(npus > 0);
  for (int pu = 0; pu < npus; ++pu) {
    const hwloc_obj_t pu_obj = hwloc_get_obj_by_depth(topology, pu_depth, pu);
    if (hwloc_bitmap_isset(cpuset, pu_obj->os_index))
      hwloc_bitmap_set(lcpuset, pu);
  }

  char lcpuset_buf[1000];
  hwloc_bitmap_list_snprintf(lcpuset_buf, sizeof lcpuset_buf, lcpuset);
  char cpuset_buf[1000];
  hwloc_bitmap_list_snprintf(cpuset_buf, sizeof cpuset_buf, cpuset);

  std::ostringstream os;
  os << " PU set L#{" << lcpuset_buf << "}"
     << " P#{" << cpuset_buf << "}";

  hwloc_bitmap_free(lcpuset);
  hwloc_bitmap_free(cpuset);

  return os.str();
}

struct cpu_info_t {
  int node, proc, nprocs, thread;
  std::string msg;

  template <typename Archive> void serialize(Archive &ar) {
    ar(node, proc, nprocs, thread, msg);
  }
};

cpu_info_t manage_affinity(const hwloc_topology_t topology) {
  const thread_layout tl;
  const thread_affinity ta(topology, tl);
  const auto set_msg = set_affinity(topology, ta);
  const auto get_msg = get_affinity(topology);

  // Busy-wait some time to prevent other threads from being scheduled
  // on the same core
  auto t0 = std::chrono::high_resolution_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now() - t0).count() < 10)
    ;

  std::ostringstream os;
  os << "FunHPC: "
     << "N" << tl.node << " "
     << "L" << tl.node_proc << " "
     << "P" << tl.proc << " "
     << "(S" << qthread_shep() << ") "
     << "T" << tl.proc_thread << set_msg << get_msg;

  return cpu_info_t{tl.node, tl.node_proc, tl.node_nprocs, tl.proc_thread,
                    os.str()};
}
}

// This routine is called on each process
void hwloc_set_affinity() {
  hwloc_topology_t topology;
  int ierr = hwloc_topology_init(&topology);
  assert(!ierr);
  ierr = hwloc_topology_load(topology);
  assert(!ierr);

  // The algorithm here fails when we yield after creating a thread.
  // Apparently, this prevents the threads to be distributed over all
  // workers. We thus temporarily disable it.
  qthread::yield_after_thread_create = false;

  int nthreads = qthread::thread::hardware_concurrency();
  std::vector<hwloc::cpu_info_t> infos(nthreads);
  bool success = false;
  int nattempts = 10;
  for (int attempt = 1; attempt <= nattempts; ++attempt) {
    int nsubmit = 10 * nthreads;

    std::vector<qthread::future<hwloc::cpu_info_t>> fs(nsubmit);
    for (auto &f : fs)
      f = qthread::async(hwloc::manage_affinity, topology);

    for (auto &info : infos)
      info = {};
    for (auto &f : fs) {
      hwloc::cpu_info_t info = f.get();
      assert(info.thread >= 0 && info.thread < nthreads);
      infos[info.thread] = info;
    }

    success = true;
    for (const auto &info : infos)
      success &= !info.msg.empty();
    if (success)
      break;
  }

  qthread::yield_after_thread_create = true;

  // Output information from the first node, sorted by MPI rank
  if (infos[0].node == 0) {
    if (infos[0].proc > 0) {
      int dummy;
      MPI_Recv(&dummy, 1, MPI_INT, infos[0].proc - 1, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    }
    for (const auto &info : infos)
      std::cout << info.msg << "\n";
    if (infos[0].proc < infos[0].nprocs - 1) {
      int dummy = 0;
      MPI_Send(&dummy, 1, MPI_INT, infos[0].proc + 1, 0, MPI_COMM_WORLD);
    }
    if (infos[0].nprocs > 1) {
      if (infos[0].proc == 0) {
        int dummy;
        MPI_Recv(&dummy, 1, MPI_INT, infos[0].nprocs - 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
      } else if (infos[0].proc == infos[0].nprocs - 1) {
        int dummy = 0;
        MPI_Send(&dummy, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      }
    }
  }

  if (!success) {
    std::cerr << "FunHPC: Could not set CPU affinity on process " << rank()
              << "\n";
    std::exit(EXIT_FAILURE);
  }

  hwloc_topology_destroy(topology);
}
}
