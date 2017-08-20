#include <qthread/mutex.hpp>
#include <qthread/thread.hpp>

#include <gtest/gtest.h>
#include <qthread.h>

#include <atomic>
#include <memory>

using namespace qthread;
using namespace std;

TEST(qthreads_mutex, basic) {
  qthread::mutex m;
  m.lock();
  m.unlock();

  {
    qthread::unique_lock<qthread::mutex> l0;
    EXPECT_EQ(nullptr, l0.mutex());
    EXPECT_FALSE(l0.owns_lock());
    EXPECT_FALSE(bool(l0));
    EXPECT_EQ(nullptr, l0.release());
    qthread::unique_lock<qthread::mutex> l1(std::move(l0));
    EXPECT_EQ(nullptr, l0.mutex());
    EXPECT_EQ(nullptr, l1.mutex());
    EXPECT_FALSE(l1.owns_lock());
    EXPECT_FALSE(bool(l1));
    EXPECT_EQ(nullptr, l1.release());

    qthread::unique_lock<qthread::mutex> l(m);
    EXPECT_EQ(&m, l.mutex());
    EXPECT_TRUE(l.owns_lock());
    EXPECT_TRUE(bool(l));
    l.unlock();
    EXPECT_EQ(&m, l.mutex());
    EXPECT_FALSE(l.owns_lock());
    EXPECT_FALSE(bool(l));
    l.lock();
    EXPECT_EQ(&m, l.mutex());
    EXPECT_TRUE(l.owns_lock());
    EXPECT_TRUE(bool(l));
    auto m1 = l.release();
    EXPECT_EQ(&m, m1);
    EXPECT_EQ(nullptr, l.mutex());
    m1->unlock();

    {
      qthread::unique_lock<qthread::mutex> ll(m);
      EXPECT_TRUE(ll.owns_lock());
      // destructing ll unlocks the mutex
    }
    m.lock();
    m.unlock();

    qthread::unique_lock<qthread::mutex> l2(m);
    qthread::unique_lock<qthread::mutex> l3(std::move(l2));
    EXPECT_EQ(nullptr, l2.mutex());
    EXPECT_EQ(&m, l3.mutex());
    swap(l2, l3);
    EXPECT_EQ(&m, l2.mutex());
    EXPECT_EQ(nullptr, l3.mutex());
    l3 = std::move(l2);
    EXPECT_EQ(nullptr, l2.mutex());
    EXPECT_EQ(&m, l3.mutex());
  }

  { qthread::lock_guard<qthread::mutex> g(m); }
}

TEST(qthreads_mutex, two_threads) {
  qthread_initialize();

  qthread::mutex m;
  atomic<int> value{0};
  m.lock();
  thread t([&]() {
    qthread::lock_guard<qthread::mutex> g(m);
    value ^= 1;
  });
  this_thread::sleep_for(std::chrono::milliseconds(100));
  value += 1;
  m.unlock();
  t.join();
  EXPECT_EQ(0, value);
}
