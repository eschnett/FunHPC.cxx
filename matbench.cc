#include "rpc.hh"

#include "algorithms.hh"
#include "block_matrix.hh"
#include "matrix.hh"

using rpc::server;

#include <boost/serialization/utility.hpp>
#include <boost/utility/identity_type.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

using std::cout;
using std::pair;
using std::ptrdiff_t;
using std::vector;



namespace bench {
  ptrdiff_t niters;
  ptrdiff_t nsize;
  ptrdiff_t nblocks;
}
using namespace bench;



typedef std::chrono::high_resolution_clock::time_point ticks;

inline ticks gettime()
{
  return std::chrono::high_resolution_clock::now();
}

double elapsed(ticks t1, ticks t0)
{
  return 1.0e-9 * std::chrono::nanoseconds(t1 - t0).count();
}



vector<int> find_all_threads()
{
  vector<int> threads;
  for (int p=0; p<server->size(); ++p) {
    // TODO: Collect thread counts from processes
    for (int t=0; t<std::thread::hardware_concurrency(); ++t) {
      threads.push_back(p);
    }
  }
  return threads;
}



typedef pair<double,double> result_t;



result_t run_dense_bench(std::ptrdiff_t n)
{
  matrix_t a(n,n), b(n,n), c(n,n);
  set(1.0, a);
  set(1.0, b);
  set(1.0, c);
  double res = 0.0;
  
  // Warmup
  gemm(false, false, 1.0, a, b, 1.0, c);
  res += nrm2(c);
  
  // Benchmark
  const auto t0 = gettime();
  for (int iter=0; iter<niters; ++iter) {
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
RPC_ACTION(run_dense_bench)

result_t run_dense_fbench(std::ptrdiff_t n)
{
  auto a = boost::make_shared<matrix_t>(n,n);
  auto b = boost::make_shared<matrix_t>(n,n);
  auto c = boost::make_shared<matrix_t>(n,n);
  set(1.0, *a);
  set(1.0, *b);
  set(1.0, *c);
  double res = 0.0;
  
  // Warmup
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += c->fnrm2();
  
  // Benchmark
  const auto t0 = gettime();
  for (int iter=0; iter<niters; ++iter) {
    c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
    res += c->fnrm2();
  }
  const auto t1 = gettime();
  
  // Cooldown
  c = a->fgemm(false, false, false, 1.0, b, 1.0, c);
  res += c->fnrm2();
  
  const double t = elapsed(t1, t0), u = niters;
  return { t, u };
}
RPC_ACTION(run_dense_fbench)



void bench_dense()
{
  const ptrdiff_t nthreads = 1;
  
  cout << "bench_dense N=" << nsize << "\n";
  
  const result_t res = run_dense_bench(nsize);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time for 1 * DGEMM[N=" << nsize << "]: " << tavg << " sec\n";
  
  cout << "This run used " << nthreads << " threads\n";
  
  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << nthreads * gflop / tavg << "\n";
  cout << "GByte/sec/core: " << nthreads * gbyte / tavg << "\n";
  
  cout << "\n";
}

void bench_fdense()
{
  const ptrdiff_t nthreads = 1;
  
  cout << "bench_fdense N=" << nsize << "\n";
  
  const result_t res = run_dense_fbench(nsize);
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time for 1 * DGEMM[N=" << nsize << "]: "
            << tavg << " sec\n";
  
  cout << "This run used " << nthreads << " threads\n";
  
  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << nthreads *gflop / tavg << "\n";
  cout << "GByte/sec/core: " << nthreads *gbyte / tavg << "\n";
  
  cout << "\n";
}

void bench_dense_parallel()
{
  vector<int> threads = find_all_threads();
  const auto nthreads = threads.size();
  
  cout << "bench_dense_parallel N=" << nsize << "\n";
  
  auto results = broadcast(threads, run_dense_bench_action(), nsize);
  result_t res = { 0.0, 0.0 };
  for (auto& fres: results) {
    const auto ires = fres.get();
    res.first += ires.first;
    res.second += ires.second;
  }
  
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time for " << nthreads << " * DGEMM[N=" << nsize << "]: "
            << tavg << " sec\n";
  
  cout << "This run used " << nthreads << " threads\n";
  
  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << nthreads * gflop / tavg << "\n";
  cout << "GByte/sec/core: " << nthreads * gbyte / tavg << "\n";
  
  cout << "\n";
}

void bench_fdense_parallel()
{
  const auto threads = find_all_threads();
  const auto nthreads = threads.size();
  
  cout << "bench_fdense_parallel N=" << nsize << "\n";
  
  auto results =
    broadcast(threads, run_dense_fbench_action(), nsize);
  result_t res = { 0.0, 0.0 };
  for (auto& fres: results) {
    const auto ires = fres.get();
    res.first += ires.first;
    res.second += ires.second;
  }
  
  const double t = res.first, u = res.second;
  const double tavg = t / u;
  cout << "CPU time for " << nthreads << " * DGEMM[N=" << nsize << "]: "
            << tavg << " sec\n";
  
  cout << "This run used " << nthreads << " threads\n";
  
  const double ops = 2.0 * pow(double(nsize), 3.0);
  const double mem = 3.0 * pow(double(nsize), 2.0) * sizeof(double);
  const double gflop = ops / 1.0e+9;
  const double gbyte = mem / 1.0e+9;
  cout << "GFlop/core:     " << gflop << "\n";
  cout << "GByte/core:     " << gbyte << "\n";
  cout << "GFlop/sec/core: " << nthreads * gflop / tavg << "\n";
  cout << "GByte/sec/core: " << nthreads * gbyte / tavg << "\n";
  
  cout << "\n";
}



int rpc_main(int argc, char** argv)
{
  niters = 3;
  nsize = 1000;
  nblocks = 10;
  
  bench_dense();
  bench_fdense();
  bench_dense_parallel();
  bench_fdense_parallel();
  // bench_fblock_local();
  // bench_fblock_global();
  
  return 0;
}
