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
void fvv() {}
void fiv(int) {}
int fvi() { return 1; }
int fii(int x) { return x; }
int frrii(int &&x) { return x; }
int frii(int &x) { return x; }
int fcrii(const int &x) { return x; }
}

TEST(funhpc_async, types) {
  auto p = 1 % size();
  auto ffvv = async(rlaunch::async, p, fvv);
  auto ffiv = async(rlaunch::async, p, fiv, 1);
  auto ffvi = async(rlaunch::async, p, fvi);
  EXPECT_EQ(1, ffvi.get());
  auto ffii = async(rlaunch::async, p, fii, 1);
  EXPECT_EQ(1, ffii.get());
  auto ffrrii = async(rlaunch::async, p, frrii, 1);
  EXPECT_EQ(1, ffrrii.get());
  // Note: async calls the function with decayed arguments, hence
  // reference arguments are not possible
  // auto ffrii = async(rlaunch::async, p, frii, 1);
  // EXPECT_EQ(1, ffrii.get());
  auto ffcrii = async(rlaunch::async, p, fcrii, 1);
  EXPECT_EQ(1, ffcrii.get());
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
