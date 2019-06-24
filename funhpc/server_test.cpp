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

int do_work(std::atomic<int> *restrict const num_active) {
  int max_active = ++*num_active;
  volatile int x = 0;
  for (int i = 0; i < 1000000; ++i)
    ++x;
  --*num_active;
  return max_active;
}

int count_threads() {
  int max_active = 0;
  for (int attempt = 0; attempt < 10; ++attempt) {
    std::atomic<int> num_active;
    num_active = 0;
    std::vector<qthread::future<int>> fs;
    for (int n = 0; n < 100; ++n)
      fs.push_back(
          qthread::async(qthread::launch::async, do_work, &num_active));
    qthread::this_thread::yield();
    for (auto &f : fs)
      max_active = std::max(max_active, f.get());
    EXPECT_EQ(0, num_active);
  }
  return max_active;
}
} // namespace

TEST(funhpc_server, disable_threading) {
  comm_lock();
  const int num_threads = qthread::thread::hardware_concurrency();
  EXPECT_FALSE(threading_disabled());
  EXPECT_EQ(num_threads, count_threads());
  threading_disable();
  EXPECT_TRUE(threading_disabled());
  EXPECT_EQ(1, count_threads());
  {
    // Test OpenMP integration
    const int n = 1000;
    int s = 0;
#pragma omp parallel for reduction(+ : s)
    for (int i = 0; i < n; ++i)
      s += i;
    EXPECT_EQ((n * n - n) / 2, s);
  }
  threading_enable();
  EXPECT_FALSE(threading_disabled());
  EXPECT_EQ(num_threads, count_threads());
  comm_unlock();
}
