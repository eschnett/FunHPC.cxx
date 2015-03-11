#include <cxx/cstdlib.hpp>
#include <funhpc/rexec.hpp>
#include <qthread/future.hpp>
#include <qthread/thread.hpp>

#include <hwloc.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace funhpc {

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

  thread_layout()
      : proc(rank()), nprocs(size()), node(proc), nnodes(nprocs),
        proc_thread(qthread::this_thread::get_worker_id()),
        proc_nthreads(qthread::thread::hardware_concurrency()),
        node_proc(cxx::div_exact(nprocs, nnodes).rem),
        node_nprocs(cxx::div_exact(nprocs, nnodes).quot),
        node_thread(node_proc * proc_nthreads + proc_thread),
        node_nthreads(node_nprocs * proc_nthreads) {
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

typedef std::tuple<int, std::string> cpu_info;

cpu_info manage_affinity(const hwloc_topology_t topology) {
  const thread_layout tl;
  const thread_affinity ta(topology, tl);
  const auto set_msg = set_affinity(topology, ta);
  const auto get_msg = get_affinity(topology);

  // Wait some time
  auto t0 = std::chrono::high_resolution_clock::now();
  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now() - t0).count() < 10)
    ;

  std::ostringstream os;
  os << "   "
     << "P" << tl.proc << " "
     << "(S" << qthread_shep() << ") "
     << "T" << tl.proc_thread << set_msg << get_msg;

  return cpu_info{tl.proc_thread, os.str()};
}
}

// This routine is called on each process
void hwloc_set_affinity() {
  hwloc_topology_t topology;
  int ierr = hwloc_topology_init(&topology);
  assert(!ierr);
  ierr = hwloc_topology_load(topology);
  assert(!ierr);

  int nthreads = qthread::thread::hardware_concurrency();
  std::vector<std::string> infos(nthreads);
  bool success = false;
  int nattempts = 10;
  for (int attempt = 1; attempt <= nattempts; ++attempt) {
    int nsubmit = 10 * nthreads;

    std::vector<qthread::future<hwloc::cpu_info>> fs(nsubmit);
    for (auto &f : fs)
      f = qthread::async(hwloc::manage_affinity, topology);

    for (auto &info : infos)
      info = {};
    for (auto &f : fs) {
      int thread;
      std::string info;
      std::tie(thread, info) = f.get();
      assert(thread >= 0 && thread < nthreads);
      infos[thread] = info;
    }

    success = true;
    for (const auto &info : infos)
      success &= !info.empty();
    if (success)
      break;
  }

  for (const auto &info : infos)
    std::cout << info << "\n";

  if (!success) {
    std::cerr << "Could not set CPU affinity on process " << rank() << "\n";
    std::exit(EXIT_FAILURE);
  }

  hwloc_topology_destroy(topology);
}
}
