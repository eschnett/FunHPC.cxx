#include <adt/array.hpp>

#include <gtest/gtest.h>

#include <numeric>
#include <type_traits>

TEST(adt_array, arithmetic) {
  std::array<int, 10> x;
  std::array<int, 10> y;
  std::iota(x.begin(), x.end(), 0);
  y.fill(1);
  x = +x;
  x = -x;
  x = ~x;
  auto b = !x;
  EXPECT_TRUE((std::is_same<decltype(b), std::array<bool, 10>>::value));
  x = x + y;
  x = x - y;
  x = x * y;
  x = x / y;
  x = x % y;
  x = x & y;
  x = x | y;
  x = x ^ y;
  b = x && y;
  b = x && y;

  x += y;
  x -= y;
  x *= y;
  x /= y;
  x %= y;
  x &= y;
  x |= y;
  x ^= y;

  abs(x);
  max(x, y);
  min(x, y);
}

TEST(adt_array, init) {
  auto z = adt::array_zero<int, 10>();
  EXPECT_EQ((std::array<int, 10>{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}), z);
  auto d = adt::array_dir<int, 10, 3>();
  EXPECT_EQ((std::array<int, 10>{{0, 0, 0, 1, 0, 0, 0, 0, 0, 0}}), d);
}

TEST(adt_array, reduce) {
  std::array<int, 10> x;
  std::iota(x.begin(), x.end(), 0);
  EXPECT_FALSE(adt::all(x));
  EXPECT_TRUE(adt::any(x));
  EXPECT_EQ(9, adt::maxval(x));
  EXPECT_EQ(0, adt::minval(x));
  EXPECT_EQ(0, adt::prod(x));
  EXPECT_EQ(45, adt::sum(x));
}
