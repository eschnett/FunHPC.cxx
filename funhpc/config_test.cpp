#include <funhpc/config.hpp>

#include <string>

#include <gtest/gtest.h>

using namespace funhpc;

TEST(funhpc_config, version) {
  EXPECT_EQ(std::string(FUNHPC_VERSION), std::string(version()));
}
