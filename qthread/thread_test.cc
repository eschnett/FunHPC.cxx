#include <qthread/thread>

#include <gtest/gtest.h>
#include <qthread.h>

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
