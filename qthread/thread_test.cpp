#include <qthread/thread.hpp>

#include <gtest/gtest.h>
#include <qthread.h>

#include <atomic>

using namespace qthread;

namespace {
int fi(int x) { return x; }
void fv(int) {}
}

TEST(qthread_thread, basic) {
  qthread_initialize();

  thread t0;
  thread t1(fi, 1);
  thread t2(fv, 1);
  EXPECT_FALSE(t0.joinable());
  EXPECT_TRUE(t1.joinable());
  EXPECT_TRUE(t2.joinable());

  thread t3(std::move(t0));
  thread t4(std::move(t1));
  EXPECT_FALSE(t3.joinable());
  EXPECT_TRUE(t4.joinable());
  EXPECT_FALSE(t0.joinable());
  EXPECT_FALSE(t1.joinable());

  t0 = std::move(t3);
  swap(t1, t4);
  EXPECT_FALSE(t0.joinable());
  EXPECT_TRUE(t1.joinable());
  EXPECT_FALSE(t3.joinable());
  EXPECT_FALSE(t4.joinable());

  EXPECT_TRUE(t1.joinable());
  t1.join();
  EXPECT_FALSE(t1.joinable());

  EXPECT_TRUE(t2.joinable());
  t2.detach();
  EXPECT_FALSE(t2.joinable());
}

namespace {
void recurse(int count, std::atomic<int> *pcounter) {
  if (count == 0)
    return;
  pcounter->fetch_add(1, std::memory_order_relaxed);
  --count;
  thread(recurse, count / 2, pcounter).detach();
  thread(recurse, count - count / 2, pcounter).detach();
}
}

TEST(qthread_thread, many) {
  int maxcount{10};
  std::atomic<int> counter{0};
  recurse(maxcount, &counter);
  while (counter < maxcount)
    this_thread::yield();
  this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(maxcount, counter);
}
