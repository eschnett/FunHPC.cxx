#include <fun/extra.hpp>

#include <gtest/gtest.h>

TEST(fun_extra, dump) {
  adt::extra<int, double> rs{42};
  std::string str(fun::dump(rs));
  EXPECT_EQ("extra{(42)}", str);
}
