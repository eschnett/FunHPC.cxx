#include <funhpc/config.hpp>

#include <gtest/gtest.h>

using namespace funhpc;

TEST(funhpc_config, version) {
  EXPECT_EQ(FUNHPC_VERSION, version());
}
