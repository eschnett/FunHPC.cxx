#include "hwloc.hh"
#include "rpc.hh"

#include "algorithms.hh"
#include "block_matrix.hh"
#include "matrix.hh"

using rpc::find_all_processes;
using rpc::find_all_threads;
using rpc::server;

#include <boost/serialization/utility.hpp>
#include <boost/utility/identity_type.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

using std::cout;
using std::min;
using std::pair;
using std::ptrdiff_t;
using std::vector;

static void reset_thread_stats() {}
RPC_ACTION(reset_thread_stats);

static std::string output_thread_stats() {
#ifdef RPC_QTHREADS
  std::ostringstream os;
  // os << "   [" << rpc::server->rank() << "] thread statistics:\n"
  //    << "      started: " << qthread::thread::threads_started << "\n"
  //    << "      running: "
  //    << qthread::thread::threads_started - qthread::thread::threads_stopped
  //    << "\n";
  return os.str();
#else
  return std::string();
#endif
}
RPC_ACTION(output_thread_stats);

namespace bench {
ptrdiff_t niters;
ptrdiff_t nsize;
ptrdiff_t bsize;
}
using namespace bench;

template <typename T> static T div_down(T x, T y) {
  RPC_ASSERT(x >= 0);
  RPC_ASSERT(y > 0);
  return x / y;
}

template <typename T> static T div_up(T x, T y) {
  RPC_ASSERT(x >= 0);
  RPC_ASSERT(y > 0);
  return (x + y - 1) / y;
}

template <typename T> static T align_up(T x, T y) {
  RPC_ASSERT(x >= 0);
  RPC_ASSERT(y > 0);
  return y * div_up(x, y);
}

template <typename T> static T align_down(T x, T y) {
  RPC_ASSERT(x >= 0);
  RPC_ASSERT(y > 0);
  return y * div_down(x, y);
}

typedef std::chrono::high_resolution_clock::time_point ticks;

inline ticks gettime() { return std::chrono::high_resolution_clock::now(); }

double elapsed(ticks t1, ticks t0) {
  return 1.0e-9 * std::chrono::nanoseconds(t1 - t0).count();
}

typedef pair<double, double> result_t;

result_t run_dense_bench(ptrdiff_t n) {
  matrix_t a(n, n), b(n, n), c(n, n);
  set(1.0, a);
  set(1.0, b);
  set(1.0, c);
  double res = 0.0;

  // Warmup
  gemm(false, false, 1.0, a, b, 1.0, c);
  res += nrm2(c);

  // Benchmark
  const auto t0 = gettime();
  for (int iter = 0; iter < niters; ++iter) {
    gemm(false, false, 1.0, a, b, 1.0, c);
    res += nrm2(c);
  }
  const auto t1 = gettime();

  // Cooldown
  gemm(false, false, 1.0, a, b, 1.0, c);
  res += nrm2(c);

  const double t = elapsed(t1, t0), u = niters;
  return { t, u };
}
RPC_ACTION(run_dense_bench);

result_t run_dense_fbench(ptrdiff_t n) {
  auto a = rpc::make_shared<matrix_t>(n, n);
  auto b = rpc::make_shared<matrix_t>(n, n);
  auto c = rpc::make_shared<matrix_t>(n, n);
  set(1.0, *a);
  set(1.0, *b);
  set(1.0, *c);
  double res = 0.0;

  // Warmup
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += *c->fnrm2();

  // Benchmark
  const auto t0 = gettime();
  for (int iter = 0; iter < niters; ++iter) {
    c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
    res += *c->fnrm2();
  }
  const auto t1 = gettime();

  // Cooldown
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += *c->fnrm2();

  const double t = elapsed(t1, t0), u = niters;
  return { t, u };
}
RPC_ACTION(run_dense_fbench);

result_t run_block_fbench(ptrdiff_t n, ptrdiff_t bs, bool run_global = false) {
  ptrdiff_t nb = div_up(n, bs);
  vector<ptrdiff_t> begin(nb + 1);
  for (ptrdiff_t b = 0; b < nb + 1; ++b) {
    begin[b] = min(n, b * div_up(n, nb));
  }
  vector<int> locs(nb * nb);
  if (!run_global) {
    for (ptrdiff_t b = 0; b < nb * nb; ++b)
      locs[b] = server->rank();
  } else {
    for (ptrdiff_t b = 0; b < nb * nb; ++b)
      locs[b] = b % server->size();
    // for (ptrdiff_t b=0; b<nb*nb; ++b) locs[b] = server->size() * b / (nb*nb);
  }
  auto str = rpc::make_shared<structure_t>(n, nb, &begin[0], &locs[0]);

  auto a = rpc::make_shared<block_matrix_t>(str, str);
  auto b = rpc::make_shared<block_matrix_t>(str, str);
  auto c = rpc::make_shared<block_matrix_t>(str, str);
  a = a->fset(false, 1.0);
  b = b->fset(false, 1.0);
  c = c->fset(false, 1.0);
  double res = 0.0;

  // Warmup
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += *c->fnrm2().make_local();

  // Benchmark
  const auto t0 = gettime();
  for (int iter = 0; iter < niters; ++iter) {
    c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
    res += *c->fnrm2().make_local();
  }
  const auto t1 = gettime();

  // Cooldown
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += *c->fnrm2().make_local();

  const double t = elapsed(t1, t0), u = niters;
  return { t, u };
}
RPC_ACTION(run_block_fbench);

void bench_dense() {
  const ptrdiff_t nthreads = 1;

  cout << "bench_dense N=" << nsize << " T=" << nthreads << "\n";

  const result_t res = run_dense_bench(nsize);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";

  cout << "\n";
}

void bench_fdense() {
  const ptrdiff_t nthreads = 1;

  cout << "bench_fdense N=" << nsize << " T=" << nthreads << "\n";

  const result_t res = run_dense_fbench(nsize);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";

  cout << "\n";
}

void bench_dense_parallel() {
  vector<int> threads = find_all_threads();
  const auto nthreads = threads.size();

  cout << "bench_dense_parallel N=" << nsize << " T=" << nthreads << "\n";

  auto results = broadcast(threads, run_dense_bench_action(), nsize);
  result_t res = { 0.0, 0.0 };
  for (auto &fres : results) {
    const auto ires = fres.get();
    res.first += ires.first;
    res.second += ires.second;
  }

  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";

  cout << "\n";
}

void bench_fdense_parallel() {
  const auto threads = find_all_threads();
  const auto nthreads = threads.size();

  cout << "bench_fdense_parallel N=" << nsize << " T=" << nthreads << "\n";

  auto results = broadcast(threads, run_dense_fbench_action(), nsize);
  result_t res = { 0.0, 0.0 };
  for (auto &fres : results) {
    const auto ires = fres.get();
    res.first += ires.first;
    res.second += ires.second;
  }

  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";

  cout << "\n";
}

void bench_fblock_local() {
  const auto nthreads = rpc::thread::hardware_concurrency();
  const auto nsize1 = lrint(nsize * sqrt(double(nthreads)));

  cout << "bench_fblock_local "
       << "N=" << nsize1 << " S=" << bsize << " T=" << nthreads << "\n";
  {
    auto fs = rpc::broadcast(find_all_threads(), reset_thread_stats_action());
    for (auto &f : fs)
      f.wait();
  }

  const result_t res = run_block_fbench(nsize1, bsize);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize1), 3.0);
  const double mem = 3.0 * pow(double(nsize1), 2.0) * sizeof(double);
  const double gflop = ops / nthreads / 1.0e+9;
  const double gbyte = mem / nthreads / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";
  {
    auto fs =
        rpc::broadcast(find_all_processes(), output_thread_stats_action());
    for (auto &f : fs)
      cout << f.get();
  }

  cout << "\n";
}

void bench_fblock_global() {
  const auto threads = find_all_threads();
  const auto nthreads = threads.size();
  const auto nsize1 = lrint(nsize * sqrt(double(nthreads)));

  cout << "bench_fblock_global "
       << "N=" << nsize1 << " S=" << bsize << " T=" << nthreads << "\n";
  {
    auto fs = rpc::broadcast(find_all_threads(), reset_thread_stats_action());
    for (auto &f : fs)
      f.wait();
  }
  {
    auto fs =
        rpc::broadcast(find_all_threads(), matrix_t::reset_stats_action());
    for (auto &f : fs)
      f.wait();
  }

  const result_t res = run_block_fbench(nsize1, bsize, true);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time/sec:   " << nthreads *tavg << "\n";
  cout << "wall time/sec:  " << tavg << "\n";

  const double ops = 2.0 * pow(double(nsize1), 3.0);
  const double mem = 3.0 * pow(double(nsize1), 2.0) * sizeof(double);
  const double gflop = ops / nthreads / 1.0e+9;
  const double gbyte = mem / nthreads / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << gflop / tavg << "\n";
  cout << "GByte/sec/core: " << gbyte / tavg << "\n";
  {
    auto fs =
        rpc::broadcast(find_all_processes(), output_thread_stats_action());
    for (auto &f : fs)
      cout << f.get();
  }
  {
    auto fs =
        rpc::broadcast(find_all_processes(), matrix_t::output_stats_action());
    for (auto &f : fs)
      cout << f.get();
  }

  cout << "\n";
}

int rpc_main(int argc, char **argv) {
#if !defined RPC_HPX
  cout << "Setting CPU bindings via hwloc:\n";
  hwloc_bindings(true);
#endif

  // best: nsize=2000, bsize=100
  // note: per-socket sheperds seem important
  niters = 3;
  nsize = 2000;
  bsize = 100;

  bench_dense();
  bench_fdense();
  bench_dense_parallel();
  bench_fdense_parallel();
  bench_fblock_local();
  bench_fblock_global();

  return 0;
}
