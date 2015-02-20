#include <funhpc/async.hpp>
#include <qthread/future.hpp>

#include <gtest/gtest.h>

using namespace funhpc;
using namespace qthread;

namespace {
int add(int x, int y) { return x + y; }
}

TEST(funhpc_async, local1) {
  auto fres = async(add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc_async, local2) {
  auto fres = async(launch::async, add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc_async, remote1) {
  auto fres = async(rlaunch::async, 0, add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc_async, remote2) {
  auto fres = async(rlaunch::async, 1 % size(), add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

namespace {
void delay() {
  qthread::this_thread::sleep_for(std::chrono::milliseconds(100));
}
int idelay() {
  qthread::this_thread::sleep_for(std::chrono::milliseconds(100));
  return 1;
}
}

TEST(funhpc_async, async) {
  auto fres = async(rlaunch::async, 1 % size(), delay);
  EXPECT_FALSE(fres.ready());
  fres.wait();
  auto ires = async(rlaunch::async, 1 % size(), idelay);
  EXPECT_FALSE(ires.ready());
  EXPECT_EQ(1, ires.get());
}

TEST(funhpc_async, sync) {
  auto fres = async(rlaunch::sync, 1 % size(), delay);
  EXPECT_TRUE(fres.ready());
  auto ires = async(rlaunch::sync, 1 % size(), idelay);
  EXPECT_TRUE(fres.ready());
  EXPECT_EQ(1, ires.get());
}

TEST(funhpc_async, deferred) {
  auto fres = async(rlaunch::deferred, 1 % size(), delay);
  EXPECT_FALSE(fres.ready());
  qthread::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_FALSE(fres.ready());
  fres.wait();
  auto ires = async(rlaunch::deferred, 1 % size(), idelay);
  EXPECT_FALSE(ires.ready());
  qthread::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_FALSE(ires.ready());
  EXPECT_EQ(1, ires.get());
}

TEST(funhpc_async, detached) {
  auto fres = async(rlaunch::detached, 1 % size(), delay);
  EXPECT_FALSE(fres.valid());
  auto ires = async(rlaunch::detached, 1 % size(), idelay);
  EXPECT_FALSE(ires.valid());
  qthread::this_thread::sleep_for(std::chrono::milliseconds(200));
}
