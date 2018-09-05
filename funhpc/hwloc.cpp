#include "hwloc.hpp"

#include <cxx/cstdlib.hpp>
#include <funhpc/rexec.hpp>
#include <qthread/future.hpp>
#include <qthread/thread.hpp>

#include <cereal/types/string.hpp>
#include <hwloc.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
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

  thread_layout() {
#if 0
    // This requires OpenMPI
    proc = cxx::envtol("OMPI_COMM_WORLD_RANK");
    assert(proc == rank());
    nprocs = cxx::envtol("OMPI_COMM_WORLD_SIZE");
    assert(nprocs == size());

    node_proc = cxx::envtol("OMPI_COMM_WORLD_LOCAL_RANK");
    node_nprocs = cxx::envtol("OMPI_COMM_WORLD_LOCAL_SIZE");
    node = cxx::div_floor(proc, node_nprocs).quot;
    nnodes = cxx::div_exact(nprocs, node_nprocs).quot;

    // TODO: also look at OMPI_UNIVERSE_SIZE
#endif

    proc = rank();
    nprocs = size();

    node_proc = local_rank();
    node_nprocs = local_size();

    node = node_rank();
    nnodes = node_size();

    proc_thread = qthread::this_thread::get_worker_id();
    proc_nthreads = qthread::thread::hardware_concurrency();

    node_thread = node_proc * proc_nthreads + proc_thread;
    node_nthreads = node_nprocs * proc_nthreads;

    const bool verbose = cxx::envtol("FUNHPC_VERBOSE", "0");
    if (verbose)
      if (proc == 0 && proc_thread == 0) {
        std::ostringstream os;
        os << "FunHPC thread layout:\n";
        os << "  proc: " << proc << " / " << nprocs << "\n";
        os << "  node_proc: " << node_proc << " / " << node_nprocs << "\n";
        os << "  node: " << node << " / " << nnodes << "\n";
        os << "  proc_thread: " << proc_thread << " / " << proc_nthreads
           << "\n";
        os << "  node_thread: " << node_thread << " / " << node_nthreads
           << "\n";
        std::cout << os.str() << std::flush;
      }

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

  thread_affinity(hwloc_topology_t topology, const thread_layout &tl) {
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

    const bool verbose = cxx::envtol("FUNHPC_VERBOSE", "0");
    if (verbose)
      if (tl.proc == 0 && tl.proc_thread == 0) {
        std::ostringstream os;
        os << "FunHPC thread affinity:\n";
        os << "  node_npus: " << node_npus << "\n";
        os << "  undersubscribing: " << undersubscribing << "\n";
        os << "  oversubscribing: " << oversubscribing << "\n";
        os << "  thread_pu: " << thread_pu << " / " << thread_npus << "\n";
        std::cout << os.str() << std::flush;
      }

    if (!oversubscribing) {
      assert(cxx::div_floor(node_npus, tl.node_nthreads).rem == 0);
      assert(cxx::div_floor(tl.node_thread * node_npus, tl.node_nthreads).rem ==
             0);
    }
    assert(invariant());
  }
};

std::string set_affinity(hwloc_topology_t topology, const thread_affinity &ta) {
  // Note: Even when undersubscribing we are binding the thread to a single PU
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  assert(pu_depth >= 0);
  const hwloc_obj_t pu_obj =
      hwloc_get_obj_by_depth(topology, pu_depth, ta.thread_pu);
  assert(pu_obj);
  // const hwloc_cpuset_t cpuset = hwloc_bitmap_dup(pu_obj->cpuset);
  const hwloc_cpuset_t cpuset = pu_obj->cpuset;

  int ierr = hwloc_set_cpubind(topology, cpuset,
                               HWLOC_CPUBIND_THREAD | HWLOC_CPUBIND_STRICT);
  if (ierr)
    ierr = hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr)
    return {" [cannot set CPU bindings]"};

  // hwloc_bitmap_free(cpuset);
  return {};
}

std::string unset_affinity(hwloc_topology_t topology,
                           const thread_affinity &ta) {
  const int machine_depth =
      hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_MACHINE);
  assert(machine_depth >= 0);
  const hwloc_obj_t machine_obj =
      hwloc_get_obj_by_depth(topology, machine_depth, 0);
  assert(machine_obj);
  const hwloc_cpuset_t cpuset = machine_obj->cpuset;

  int ierr = hwloc_set_cpubind(topology, cpuset,
                               HWLOC_CPUBIND_THREAD | HWLOC_CPUBIND_STRICT);
  if (ierr)
    ierr = hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr)
    return {" [cannot set CPU bindings]"};

  return {};
}

std::string get_affinity(hwloc_topology_t topology) {
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

  char *lcpuset_buf;
  hwloc_bitmap_list_asprintf(&lcpuset_buf, lcpuset);
  char *cpuset_buf;
  hwloc_bitmap_list_asprintf(&cpuset_buf, cpuset);

  std::ostringstream os;
  os << " PU set L#{" << lcpuset_buf << "} P#{" << cpuset_buf << "}";

  free(lcpuset_buf);
  free(cpuset_buf);
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

cpu_info_t manage_affinity(hwloc_topology_t topology, bool do_set_affinity,
                           bool do_unset_affinity) {
  thread_layout tl;
  thread_affinity ta(topology, tl);
  const auto set_msg = do_set_affinity ? set_affinity(topology, ta) : "";
  const auto unset_msg = do_unset_affinity ? unset_affinity(topology, ta) : "";
  const auto get_msg = get_affinity(topology);

  std::ostringstream os;
  os << "FunHPC[" << rank() << "]: "
     << "N" << tl.node << " "
     << "L" << tl.node_proc << " "
     << "P" << tl.proc << " "
     << "(S" << qthread_shep() << ") "
     << "T" << tl.proc_thread << set_msg << get_msg;

  return cpu_info_t{tl.node, tl.node_proc, tl.node_nprocs, tl.proc_thread,
                    os.str()};
}

std::vector<cpu_info_t> cpu_infos;

// This routine is called on each process
void set_all_cpu_affinities() {
  const bool set_thread_bindings =
      cxx::envtol("FUNHPC_SET_THREAD_BINDINGS", "1");
  const bool unset_thread_bindings =
      cxx::envtol("FUNHPC_UNSET_THREAD_BINDINGS", "0");

  hwloc_topology_t topology;
  int ierr = hwloc_topology_init(&topology);
  assert(!ierr);
  ierr = hwloc_topology_load(topology);
  assert(!ierr);

  const int nthreads = qthread::thread::hardware_concurrency();
  cpu_infos.resize(nthreads);

  qthread::all_threads::run([&]() {
    const int thread = qthread::this_thread::get_worker_id();
    cpu_infos.at(thread) =
        manage_affinity(topology, set_thread_bindings, unset_thread_bindings);
  });

  hwloc_topology_destroy(topology);
}

std::string get_all_cpu_infos() {
  std::ostringstream os;
  for (const auto &cpu_info : cpu_infos)
    os << cpu_info.msg << "\n";
  return os.str();
}
} // namespace hwloc
} // namespace funhpc
