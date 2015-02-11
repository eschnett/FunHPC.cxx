#include <funhpc/async>
#include <qthread/future>

#include <gtest/gtest.h>

using namespace funhpc;
using namespace qthread;

int add(int x, int y) { return x + y; }

TEST(funhpc, async_local1) {
  auto fres = async(add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc, async_local2) {
  auto fres = async(launch::async, add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc, async_remote1) {
  auto fres = async(rlaunch::async, 0, add, 1, 2);
  EXPECT_EQ(3, fres.get());
}

TEST(funhpc, async_remote2) {
  auto fres = async(rlaunch::async, 1 % size(), add, 1, 2);
  EXPECT_EQ(3, fres.get());
}
