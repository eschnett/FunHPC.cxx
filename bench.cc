#include "rpc.hh"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

using rpc::async;
using rpc::future;
using rpc::thread;

using std::atomic;
using std::cout;
using std::endl;
using std::flush;
using std::min;
using std::ptrdiff_t;
using std::vector;



typedef std::chrono::high_resolution_clock::time_point ticks;

inline ticks gettime()
{
  return std::chrono::high_resolution_clock::now();
}

double elapsed(ticks t1, ticks t0)
{
  return 1.0e-9 * std::chrono::nanoseconds(t1 - t0).count();
}



enum continuation_t { do_nothing,
                      do_daisychain_func, do_daisychain,
                      do_tree_func, do_tree };

double busywait_rate = 1.0e+9;  // operations per second

void run_next(double time, continuation_t cont, ptrdiff_t njobs);

atomic<ptrdiff_t> busywait_count;
atomic<double> busywait_result;
void busywait(double time, continuation_t cont, ptrdiff_t njobs)
{
  assert(njobs >= 0);
  if (njobs == 0) return;
#if 0
  const auto t0 = gettime();
  for (;;) {
    const auto t1 = gettime();
    if (elapsed(t1, t0) >= time) break;
  }
#else
  ptrdiff_t count = llrint(time * busywait_rate);
  double eps = 1.0e-10;
  double x = 1.0;
  for (ptrdiff_t i=0; i<count; ++i) x = x * (1.0+eps) + eps;
  // volatile double y __attribute__((__unused__)) = x;
  ++busywait_count;
  busywait_result = x;
#endif
  run_next(time, cont, njobs-1);
}
struct busywait_action:
  public rpc::action_impl<busywait_action,
                          rpc::wrap<decltype(&busywait), &busywait>>
{
};
BOOST_CLASS_EXPORT(busywait_action::evaluate);
BOOST_CLASS_EXPORT(busywait_action::finish);

void run_next(double time, continuation_t cont, ptrdiff_t njobs)
{
  switch (cont) {
  default:
    __builtin_unreachable(); assert(0);
  case do_nothing:
    break;
  case do_daisychain_func: {
    auto f = async(busywait, time, cont, njobs);
    f.wait();
    break;
  }
  case do_daisychain: {
    const auto here = rpc::server->rank();
    auto f = rpc::async(here, busywait_action(), time, cont, njobs);
    f.wait();
    break;
  }
  case do_tree_func: {
    auto njobs1 = (njobs+1) / 2;
    auto njobs2 = njobs - njobs1;
    auto f1 = async(busywait, time, cont, njobs1);
    auto f2 = async(busywait, time, cont, njobs2);
    f1.wait();
    f2.wait();
    break;
  }
  case do_tree: {
    const auto here = rpc::server->rank();
    auto njobs1 = (njobs+1) / 2;
    auto njobs2 = njobs - njobs1;
    auto f1 = rpc::async(here, busywait_action(), time, cont, njobs1);
    auto f2 = rpc::async(here, busywait_action(), time, cont, njobs2);
    f1.wait();
    f2.wait();
    break;
  }
  }
}

void calibrate_busywait()
{
  busywait_count = 0;
  busywait_result = 0.0;
  busywait_rate = 1.0e+9;       // irrelevant but non-zero
  auto count = 1.0e+6;          // choice
  auto t0 = gettime();
  busywait(count / busywait_rate, do_nothing, 1);
  auto t1 = gettime();
  auto dt = elapsed(t1, t0);
  auto flop_sec = 2.0 * count / dt;
  cout << "Busy-waiting with " << flop_sec / 1.0e+9 << " Gflop/sec\n";
  busywait_rate = count / dt;
}



void run_sync_func(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: function sync...       " << flush;
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    busywait(time, do_nothing, 1);
  }
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_sync(double time, ptrdiff_t njobs,
              const vector<int>& locs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action sync...         " << flush;
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    rpc::sync(locs[n % locs.size()], busywait_action(), time, do_nothing, 1);
  }
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_single(double time, ptrdiff_t njobs)
{
  for (ptrdiff_t n=0; n<njobs; ++n) {
    busywait(time, do_nothing, 1);
  }
}
struct run_single_action:
  public rpc::action_impl<run_single_action,
                          rpc::wrap<decltype(&run_single), &run_single>>
{
};
BOOST_CLASS_EXPORT(run_single_action::evaluate);
BOOST_CLASS_EXPORT(run_single_action::finish);

void run_multi_func(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: function multi...      " << flush;
  vector<future<void>> fs(nthreads);
  const auto t0 = gettime();
  auto njobs_left = njobs;
  const auto njobs1 = (njobs + nthreads - 1) / nthreads;
  for (ptrdiff_t t=0; t<nthreads; ++t) {
    const auto njobs2 = min(njobs1, njobs_left);
    njobs_left -= njobs2;
    fs[t] = async(run_single, time, njobs2);
  }
  assert(njobs_left == 0);
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_multi(double time, ptrdiff_t njobs,
               const vector<int>& locs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action multi...        " << flush;
  assert(nthreads == ptrdiff_t(locs.size() * thread::hardware_concurrency()));
  vector<future<void>> fs(nthreads);
  const auto t0 = gettime();
  auto njobs_left = njobs;
  const auto njobs1 = (njobs + nthreads - 1) / nthreads;
  for (ptrdiff_t t=0; t<nthreads; ++t) {
    const auto njobs2 = min(njobs1, njobs_left);
    njobs_left -= njobs2;
    fs[t] = rpc::async(locs[t % locs.size()], run_single_action(),
                       time, njobs2);
  }
  assert(njobs_left == 0);
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_serial_func(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: function serial...     " << flush;
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    auto f = async(busywait, time, do_nothing, 1);
    f.wait();
  }
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_serial(double time, ptrdiff_t njobs,
                const vector<int>& locs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action serial...       " << flush;
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    auto f = rpc::async(locs[n % locs.size()], busywait_action(),
                        time, do_nothing, 1);
    f.wait();
  }
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_daisychain_func(double time, ptrdiff_t njobs,
                         ptrdiff_t nthreads)
{
  cout << "    Configuration: function daisychain... " << flush;
  const auto t0 = gettime();
  run_next(time, do_daisychain_func, njobs);
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_daisychain(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action daisychain...   " << flush;
  const auto t0 = gettime();
  run_next(time, do_daisychain, njobs);
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_parallel_func(double time, ptrdiff_t njobs,
                       ptrdiff_t nthreads)
{
  cout << "    Configuration: function parallel...   " << flush;
  vector<future<void>> fs(njobs);
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    fs[n] = async(busywait, time, do_nothing, 1);
  }
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_parallel(double time, ptrdiff_t njobs,
                  const vector<int>& locs,
                  ptrdiff_t nthreads)
{
  cout << "    Configuration: action parallel...     " << flush;
  vector<future<void>> fs(njobs);
  const auto t0 = gettime();
  for (ptrdiff_t n=0; n<njobs; ++n) {
    fs[n] = rpc::async(locs[n % locs.size()], busywait_action(),
                       time, do_nothing, 1);
  }
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_tree_func(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: function tree...       " << flush;
  const auto t0 = gettime();
  run_next(time, do_tree_func, njobs);
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_tree(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action tree...         " << flush;
  const auto t0 = gettime();
  run_next(time, do_tree, njobs);
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_sqrt_func(double time, ptrdiff_t njobs, ptrdiff_t nthreads)
{
  cout << "    Configuration: function sqrt...       " << flush;
  const ptrdiff_t njobs_par = lrint(sqrt(njobs));
  vector<future<void>> fs(njobs_par);
  const auto t0 = gettime();
  auto njobs_left = njobs;
  const auto njobs1 = (njobs + njobs_par - 1) / njobs_par;
  for (ptrdiff_t n=0; n<njobs_par; ++n) {
    const auto njobs2 = min(njobs1, njobs_left);
    njobs_left -= njobs2;
    fs[n] = async(busywait, time, do_daisychain_func, njobs2);
  }
  assert(njobs_left == 0);
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}

void run_sqrt(double time, ptrdiff_t njobs,
              const vector<int>& locs, ptrdiff_t nthreads)
{
  cout << "    Configuration: action sqrt...         " << flush;
  const ptrdiff_t njobs_par = lrint(sqrt(njobs));
  vector<future<void>> fs(njobs_par);
  const auto t0 = gettime();
  auto njobs_left = njobs;
  const auto njobs1 = (njobs + njobs_par - 1) / njobs_par;
  for (ptrdiff_t n=0; n<njobs_par; ++n) {
    const auto njobs2 = min(njobs1, njobs_left);
    njobs_left -= njobs2;
    fs[n] = rpc::async(locs[n % locs.size()], busywait_action(),
                       time, do_daisychain, njobs2);
  }
  assert(njobs_left == 0);
  for (auto& f: fs) f.wait();
  const auto t1 = gettime();
  const double usec = elapsed(t1, t0) * nthreads / njobs * 1.0e+6;
  cout << " " << usec << " µsec" << endl;
}



void rpcbench_local_functions()
{
  const ptrdiff_t njobs = 1000;
  const ptrdiff_t nthreads = thread::hardware_concurrency();
  cout << "RPC local function benchmarks: "
       << "N=" << njobs << ", T=" << nthreads << endl;
  
  const double times[] = {0.0, 10.0e-6, 100.0e-6};
  
  for (auto time: times) {
    cout << "  Benchmark: busywait (" << time * 1.0e+6 << " µsec)"
              << endl;
    run_sync_func(time, njobs, nthreads);
    run_multi_func(time, njobs, nthreads);
    run_serial_func(time, njobs, nthreads);
    run_daisychain_func(time, njobs, nthreads);
    run_parallel_func(time, njobs, nthreads);
    run_tree_func(time, njobs, nthreads);
    run_sqrt_func(time, njobs, nthreads);
  }
  
  cout << endl;
}

void rpcbench_local_actions()
{
  const ptrdiff_t njobs = 1000;
  const ptrdiff_t nthreads = thread::hardware_concurrency();
  cout << "RPC local action benchmarks: "
       << "N=" << njobs << ", T=" << nthreads << endl;
  const auto here = rpc::server->rank();
  const vector<int> locs(1, here);
  
  const double times[] = {0.0, 10.0e-6, 100.0e-6};
  
  for (auto time: times) {
    cout << "  Benchmark: busywait (" << time * 1.0e+6 << " µsec)"
              << endl;
    run_sync(time, njobs, locs, nthreads);
    run_multi(time, njobs, locs, nthreads);
    run_serial(time, njobs, locs, nthreads);
    run_daisychain(time, njobs, nthreads);
    run_parallel(time, njobs, locs, nthreads);
    run_tree(time, njobs, nthreads);
    run_sqrt(time, njobs, locs, nthreads);
  }
  
  cout << endl;
}

void rpcbench_global_actions()
{
  const ptrdiff_t njobs = 1000;
  const ptrdiff_t nthreads =
    rpc::server->size() * thread::hardware_concurrency();
  cout << "RPC global action benchmarks: "
       << "N=" << njobs << ", T=" << nthreads << endl;
  vector<int> locs(rpc::server->size());
  for (int dest=0; dest<rpc::server->size(); ++dest) locs[dest] = dest;
  
  const double times[] = {0.0, 10.0e-6, 100.0e-6};
  
  for (auto time: times) {
    cout << "  Benchmark: busywait (" << time * 1.0e+6 << " µsec)"
         << endl;
    run_sync(time, njobs, locs, nthreads);
    run_multi(time, njobs, locs, nthreads);
    run_serial(time, njobs, locs, nthreads);
    // run_daisychain(time, njobs, nthreads);
    run_parallel(time, njobs, locs, nthreads);
    // run_tree(time, njobs, nthreads);
    run_sqrt(time, njobs, locs, nthreads);
  }
  
  cout << endl;
}



int rpc_main(int argc, char** argv)
{
  const auto t0 = gettime();
  const auto t1 = gettime();
  const double dt = elapsed(t1, t0);
  cout << "Timing overhead: " << dt * 1.0e+6 << " µsec" << endl;
  calibrate_busywait();
  cout << endl;
  
  rpcbench_local_functions();
  rpcbench_local_actions();
  if (rpc::server->size() > 1) {
    rpcbench_global_actions();
  }
  
  return 0;
}
