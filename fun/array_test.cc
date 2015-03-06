#include <fun/array.hpp>

#include <fun/fun.hpp>

#include <gtest/gtest.h>

#include <algorithm>

using namespace fun;

template <typename T> using array0 = std::array<T, 0>;
template <typename T> using array10 = std::array<T, 10>;

TEST(fun_array, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<array10>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), std::array<int, 10>>::value, "");
  EXPECT_EQ(s, rs.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i, rs[i]);

  auto rs1 =
      iotaMap<array10>([](auto x, auto y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), std::array<double, 10>>::value, "");
  EXPECT_EQ(s, rs1.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i - 1, rs1[i]);
}

TEST(fun_array, fmap) {
  const std::ptrdiff_t s = 10;
  std::array<int, s> xs;
  for (std::ptrdiff_t i = 0; i < s; ++i)
    xs[i] = i;

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_EQ(s, rs.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i + 1, rs[i]);

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_EQ(s, rs2.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i + 2, rs2[i]);

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_EQ(rs3.size(), s);
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(2 * i + 1, rs3[i]);

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ((s - 1) * s / 2, accum);
}

TEST(fun_array, fmapTopo) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<array10>([](int x) { return x * x; }, s);
  auto ys = fmapTopo(
      [](auto x, const auto &bs) { return get<0>(bs) - 2 * x + get<1>(bs); },
      [](auto x, auto i) { return x; }, xs, connectivity<int>(1, 100));
  auto sum = foldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, ys);
  EXPECT_EQ(20, sum);
}

TEST(fun_array, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<array10>([](auto x) { return int(x); }, s);
  auto ys = xs;

  auto sum = foldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, xs);
  static_assert(std::is_same<decltype(sum), int>::value, "");
  EXPECT_EQ((s - 1) * s / 2, sum);

  auto sum2 = foldMap2([](auto x, auto y) { return x + y; },
                       [](auto x, auto y) { return x + y; }, 0, xs, ys);
  static_assert(std::is_same<decltype(sum2), int>::value, "");
  EXPECT_EQ((s - 1) * s, sum2);

  auto sum_sq = foldMap([](auto x) { return x * x; },
                        [](auto x, auto y) { return x + y; }, 0, xs);
  static_assert(std::is_same<decltype(sum), int>::value, "");
  EXPECT_EQ((s - 1) * s * (2 * s - 1) / 6, sum_sq);
}

TEST(fun_array, monad) {
  auto x1 = munit<array10>(1);
  static_assert(std::is_same<decltype(x1), std::array<int, 10>>::value, "");
  EXPECT_EQ(10, x1.size());
  EXPECT_EQ(1, x1[0]);

  auto xx1 = munit<array10>(x1);
  EXPECT_EQ(10, xx1.size());
  EXPECT_EQ(10, xx1[0].size());
  EXPECT_EQ(1, xx1[0][0]);

  auto x1j = mjoin(xx1);
  // EXPECT_EQ(x1, x1j);

  auto x2 = mbind([](auto x, auto c) { return munit<array10>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), std::array<int, 100>>::value, "");
  EXPECT_EQ(100, x2.size());
  EXPECT_EQ(2, x2[0]);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(10, r1.size());
  EXPECT_EQ(r1.size() * r, mextract(r1));

  auto x0 = mzero<array0, int>();
  static_assert(std::is_same<decltype(x0), std::array<int, 0>>::value, "");
  EXPECT_TRUE(x0.empty());

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1[0]));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
}
