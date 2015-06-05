#include <cxx/cstdlib.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
void check_div_floor(int x, int y) {
  // x == q*y+r
  // q <= x/y
  auto div = cxx::div_floor(x, y);
  int q = div.quot;
  int r = div.rem;
  EXPECT_TRUE(x == q * y + r);
  EXPECT_TRUE(std::abs(r) < std::abs(y));
  EXPECT_TRUE(q <= double(x) / double(y));
  EXPECT_TRUE(q + 1 > double(x) / double(y));
  EXPECT_TRUE(q == std::floor(double(x) / double(y)));
  if (y > 0) {
    EXPECT_TRUE(q * y <= x);
    EXPECT_TRUE((q + 1) * y > x);
    EXPECT_TRUE(r >= 0 && r < y);
  } else {
    EXPECT_TRUE(q * y >= x);
    EXPECT_TRUE((q + 1) * y < x);
    EXPECT_TRUE(r <= 0 && r > y);
  }
}

void check_div_ceil(int x, int y) {
  // x == q*y+r
  // q >= x/y
  auto div = cxx::div_ceil(x, y);
  int q = div.quot;
  int r = div.rem;
  EXPECT_TRUE(x == q * y + r);
  EXPECT_TRUE(std::abs(r) < std::abs(y));
  EXPECT_TRUE(q >= double(x) / double(y));
  EXPECT_TRUE(q - 1 < double(x) / double(y));
  EXPECT_TRUE(q == std::ceil(double(x) / double(y)));
  if (y > 0) {
    EXPECT_TRUE(q * y >= x);
    EXPECT_TRUE((q - 1) * y < x);
    EXPECT_TRUE(r <= 0 && r > -y);
  } else {
    EXPECT_TRUE(q * y <= x);
    EXPECT_TRUE((q - 1) * y > x);
    EXPECT_TRUE(r >= 0 && r < -y);
  }
}

void check_div_exact(int x, int y) {
  // x == q*y+r
  // q == x/y
  auto div = cxx::div_exact(x, y);
  int q = div.quot;
  int r = div.rem;
  EXPECT_TRUE(x == q * y + r);
  EXPECT_TRUE(r == 0);
  EXPECT_TRUE(q == double(x) / double(y));
  EXPECT_TRUE(q * y == x);
}
}

TEST(cxx_cstdlib, div) {
  for (int x = -10; x <= +10; ++x) {
    for (int y = -10; y <= +10; ++y) {
      if (y != 0) {
        check_div_floor(x, y);
        check_div_ceil(x, y);
        if (x % y == 0) {
          check_div_exact(x, y);
        }
      }
    }
  }
}

TEST(cxx_cstdlib, ipow) {
  for (int x = -10; x <= +9; ++x) {
    for (int y = 1; y <= +9; ++y) {
      EXPECT_EQ(std::pow(x, y), double(cxx::ipow(x, y)));
    }
  }
}
