#include <qthread/mutex>
#include <qthread/thread>

#include <gtest/gtest.h>
#include <qthread.h>

#include <memory>

using namespace qthread;

TEST(qthreads_mutex, basic) {
  mutex m;
  m.lock();
  m.unlock();

  { lock_guard<mutex> g(m); }
}

TEST(qthreads_mutex, two_threads) {
  qthread_initialize();

  mutex m;
  int value = 0;
  m.lock();
  thread t([&]() {
    lock_guard<mutex> g(m);
    value *= 2;
  });
  this_thread::sleep_for(std::chrono::milliseconds(100));
  value += 1;
  m.unlock();
  t.join();
  EXPECT_EQ(value, 2);
}
