#include <cxx/task>

#include <gtest/gtest.h>

using namespace cxx;

TEST(cxx_task, task) {
  auto t0v = task<void>([]() {});
  auto t1v = task<void>([](int) {}, 1);
  auto t2v = task<void>([](int x) { return x; }, 1);
  auto t3v = task<void>([]() { return 1; });
  auto t2i = task<int>([](int x) { return x; }, 1);
  auto t3i = task<int>([]() { return 1; });
  t0v();
  t1v();
  t2v();
  t3v();
  auto r2i = t2i();
  auto r3i = t3i();
  EXPECT_EQ(1, r2i);
  EXPECT_EQ(1, r3i);

  // We should have many more tests here. We don't, but that's
  // acceptable since task is used (and thus tested) extensively in
  // qthread/future.
}
