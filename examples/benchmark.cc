#include <cxx/invoke.hpp>
#include <funhpc/main.hpp>
#include <qthread/future.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <utility>
#include <vector>

template <typename T> T clamp(T x, T minval, T maxval) {
  return std::min(std::max(x, minval), maxval);
}

double gettime() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec / 1.0e+6;
}

typedef double token;
double begin_work() {
  volatile double x{1.0};
  return x;
}
double do_workitem(double x) { return sqrt(x); }
void finish_work(double x) {
  volatile double r;
  volatile double *rp = &r;
  *rp = x;
}

token do_work(token tok, std::int64_t items) {
  for (std::int64_t i = 0; i < items; ++i)
    tok = do_workitem(tok);
  return tok;
}

token serial(token tok, std::int64_t items, std::int64_t iters) {
  for (std::ptrdiff_t i = 0; i < iters; ++i)
    tok = do_work(tok, items);
  return tok;
}

token parallel(token tok, std::int64_t items, std::int64_t iters) {
  std::vector<qthread::future<token>> fs(iters);
  for (auto &f : fs)
    f = qthread::async(do_work, tok, items);
  for (auto &f : fs)
    tok += f.get();
  return tok;
}

token daisychained(token tok, std::int64_t items, std::int64_t iters) {
  if (iters == 0)
    return tok;
  --iters;
  tok = do_work(tok, items);
  auto f = qthread::async(daisychained, tok, items, iters);
  return f.get();
}

token tree(token tok, std::int64_t items, std::int64_t iters) {
  if (iters == 0)
    return tok;
  if (iters == 1)
    return do_work(tok, items);
  auto iters1 = iters / 2;
  auto iters2 = iters - iters1;
  auto f1 = qthread::async(tree, tok, items, iters1);
  auto f2 = qthread::async(tree, tok, items, iters2);
  return f1.get() + f2.get();
}

template <typename F> void runbench(const std::string &name, F &&f) {
  std::int64_t workitemss[] = {1, 1000, 1000000};
  std::int64_t inititers = 100;
  double mintime = 1.0;

  for (auto workitems : workitemss) {
    std::ostringstream os;
    os << name << ", " << workitems << " workitems:";
    auto str = os.str();
    std::cout << "   " << std::left << std::setw(32) << str << std::flush;
    auto iters = inititers;
    auto time = mintime;
    for (;;) {
      token tok = begin_work();
      auto t0 = gettime();
      tok = cxx::invoke(f, tok, workitems, iters);
      auto t1 = gettime();
      finish_work(tok);
      time = t1 - t0;
      if (time >= mintime)
        break;
      iters = clamp(std::int64_t(llrint(1.1 * iters * mintime / time)),
                    2 * iters, 10 * iters);
    }
    std::cout << "   " << time / iters * 1.0e+6 << " usec/iter, "
              << time / (iters * workitems) * 1.0e+9 << " nsec/iter/item   ("
              << iters << " iters, " << time << " sec)\n";
  }
  std::cout << "\n";
}

int funhpc_main(int argc, char **argv) {
  std::cout << "Single-Node Benchmark\n"
            << "\n";

  runbench("serial", serial);
  runbench("daisychained", daisychained);
  runbench("parallel", parallel);
  runbench("tree", tree);

  std::cout << "Done.\n";
  return 0;
}
