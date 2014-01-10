#include "rpc.hh"

#include <hwloc.h>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>



using std::atomic;
using std::cerr;
using std::cout;
using std::getenv;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::stoi;
using std::string;
using std::vector;



int stoi1(const char* str, int dflt)
{
  if (str) {
    try {
      const string str1(str);
      size_t pos;
      int res = stoi(str1, &pos);
      if (pos == str1.length()) return res;
    } catch (...) {
      // do nothing
    }
  }
  return dflt;
}

void output_affinity(ostream& os, const hwloc_topology_t& topology)
{
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth>=0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus>0);
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  RPC_ASSERT(cpuset);
  const int ierr = hwloc_get_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  if (ierr) {
    os << "   Could not determine CPU bindings\n";
    hwloc_bitmap_free(cpuset);
    return;
  }
  hwloc_cpuset_t lcpuset = hwloc_bitmap_alloc();
  RPC_ASSERT(lcpuset);
  for (int pu_num=0; pu_num<num_pus; ++pu_num) {
    const hwloc_obj_t pu_obj =
      hwloc_get_obj_by_depth(topology, pu_depth, pu_num);
    if (hwloc_bitmap_isset(cpuset, pu_obj->os_index)) {
      hwloc_bitmap_set(lcpuset, pu_num);
    }
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
}

void set_affinity(ostream& os, const hwloc_topology_t& topology)
{
  const int rank = stoi1(getenv("OMPI_COMM_WORLD_RANK"), rpc::server->rank());
  const int size = stoi1(getenv("OMPI_COMM_WORLD_SIZE"), rpc::server->size());
  const int local_rank = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_RANK"), 0);
  const int local_size = stoi1(getenv("OMPI_COMM_WORLD_LOCAL_SIZE"), 1);
  RPC_ASSERT(rank>=0 && rank<size);
  RPC_ASSERT(local_rank>=0 && local_rank<local_size);
  RPC_ASSERT(rank>=local_rank && local_size<=size);
  
  const int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  RPC_ASSERT(pu_depth>=0);
  const int num_pus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  RPC_ASSERT(num_pus>0);
  
  const int thread = rpc::this_thread::get_worker_id();
  const int num_threads = rpc::thread::hardware_concurrency();
  const int node_num_threads = local_size * num_threads;
  const int node_thread = local_rank * num_threads + thread;
  
  const bool oversubscribing = node_num_threads > num_pus;
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
  if (ierr) {
    ierr = hwloc_set_cpubind(topology, cpuset, HWLOC_CPUBIND_THREAD);
  }
  if (ierr) {
    os << "Could not set CPU binding for thread " << thread << "\n";
  }
  hwloc_bitmap_free(cpuset);
}

pair<int,string> hwloc_run(bool do_set, atomic<int>* pcount)
{
  atomic<int>& count = *pcount;
  ostringstream os;
  const int thread = rpc::this_thread::get_worker_id();
  const int nthreads = rpc::thread::hardware_concurrency();
  // Wait until all threads are running to ensure all threads are
  // running simultaneously, and thus are running on hardware threads
  RPC_ASSERT(count < nthreads);
  ++count;
  while (count < nthreads); // busy-wait for all threads to arrive
  RPC_ASSERT(count == nthreads);
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  if (do_set) {
    set_affinity(os, topology);
  }
  output_affinity(os, topology);
  return { thread, os.str() };
}

string hwloc_run_on_threads(bool do_set)
{
  ostringstream os;
  int nthreads = rpc::thread::hardware_concurrency();
  const auto here = rpc::server->rank();
  vector<rpc::future<pair<int,string> > > fs;
  atomic<int> count;
  count = 0;
  for (int submit=0; submit<nthreads; ++submit) {
    fs.push_back(rpc::async(hwloc_run, do_set, &count));
  }
  vector<string> infos(nthreads);
  for (auto& f: fs) {
    const auto thread_info = f.get();
    const auto thread = thread_info.first;
    const auto info = thread_info.second;
    RPC_ASSERT(thread>=0 && thread<nthreads);
    RPC_ASSERT(infos[thread].empty());
    infos[thread] = info;
  }
  RPC_ASSERT(count == nthreads);
  for (const auto& info: infos) {
    os << info;
  }
  return os.str();
}
RPC_ACTION(hwloc_run_on_threads);

void hwloc_run_on_processes(bool do_set)
{
  auto fs = rpc::broadcast(rpc::find_all_processes(),
                           hwloc_run_on_threads_action(), do_set);
  for (auto& f: fs) {
    cout << f.get();
  }
}

int rpc_main(int argc, char** argv)
{
  cout << "HWLOC information:\n";
  hwloc_run_on_processes(false);
  cout << "Setting CPU bindings:\n";
  hwloc_run_on_processes(true);
  return 0;
}
