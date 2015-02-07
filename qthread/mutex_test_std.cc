#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <thread>

using namespace std;

TEST(std_mutex, basic) {
  mutex m;
  m.lock();
  m.unlock();

  { lock_guard<mutex> g(m); }
}

TEST(std_mutex, two_threads) {
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
