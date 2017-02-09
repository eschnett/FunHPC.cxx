#include <funhpc/server.hpp>

#include <gtest/gtest.h>
#include <qthread/future.hpp>
#include <qthread/thread.hpp>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <vector>

using namespace funhpc;

namespace {
std::atomic<int> num_active;

int do_work() {
  int max_active = ++num_active;
  volatile int x = 0;
  for (int i = 0; i < 1000000; ++i)
    ++x;
  --num_active;
  return max_active;
}

int count_threads() {
  std::vector<qthread::future<int>> fs;
  for (int n = 0; n < 100; ++n)
    fs.push_back(qthread::async(qthread::launch::async, do_work));
  qthread::this_thread::yield();
  int max_active = 0;
  for (auto &f : fs)
    max_active = std::max(max_active, f.get());
  return max_active;
}
}

TEST(funhpc_server, disable_threading) {
  std::cout << "nthreads=" << count_threads() << "\n" << std::flush;
  threading_lock();
  std::cout << "nthreads=" << count_threads() << "\n" << std::flush;
  int max_active = count_threads();
  EXPECT_EQ(1, max_active);
  // Test OpenMP integration
  const int n = 1000;
  int s = 0;
#pragma omp parallel for reduction(+ : s)
  for (int i = 0; i < n; ++i)
    s += i;
  EXPECT_EQ((n * n - n) / 2, s);
  threading_unlock();
  std::cout << "nthreads=" << count_threads() << "\n" << std::flush;
}
