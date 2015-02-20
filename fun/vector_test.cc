#include <fun/vector.hpp>

#include <gtest/gtest.h>

using namespace fun;

template <typename T> using vector1 = std::vector<T>;

TEST(fun_vector, iota) {
  std::size_t s = 10;
  auto rs = iota<std::vector>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), std::vector<int>>::value, "");
  EXPECT_EQ(s, rs.size());
  for (std::size_t i = 0; i < s; ++i)
    EXPECT_EQ(i, rs[i]);

  auto rs1 = iota<vector1>([](auto x, auto y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), std::vector<double>>::value, "");
  EXPECT_EQ(s, rs1.size());
  for (std::size_t i = 0; i < s; ++i)
    EXPECT_EQ(i - 1, rs1[i]);
}

TEST(fun_vector, fmap) {
  std::size_t s = 10;
  std::vector<int> xs(s);
  for (std::size_t i = 0; i < s; ++i)
    xs[i] = i;

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_EQ(s, rs.size());
  for (std::size_t i = 0; i < s; ++i)
    EXPECT_EQ(i + 1, rs[i]);

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_EQ(s, rs2.size());
  for (std::size_t i = 0; i < s; ++i)
    EXPECT_EQ(i + 2, rs2[i]);

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_EQ(rs3.size(), s);
  for (std::size_t i = 0; i < s; ++i)
    EXPECT_EQ(2 * i + 1, rs3[i]);

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ((s - 1) * s / 2, accum);
}

TEST(fun_vector, foldMap) {
  std::size_t s = 10;
  auto xs = iota<std::vector>([](auto x) { return x; }, s);
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

TEST(fun_vector, monad) {
  auto x1 = munit<std::vector>(1);
  static_assert(std::is_same<decltype(x1), std::vector<int>>::value, "");
  EXPECT_EQ(1, x1.size());
  EXPECT_EQ(1, x1[0]);

  auto xx1 = munit<std::vector>(x1);
  EXPECT_EQ(1, xx1.size());
  EXPECT_EQ(1, xx1[0].size());
  EXPECT_EQ(1, xx1[0][0]);

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind([](auto x, auto c) { return munit<vector1>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), std::vector<int>>::value, "");
  EXPECT_EQ(1, x2.size());
  EXPECT_EQ(2, x2[0]);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto x0 = mzero<std::vector, int>();
  auto x0a = mzero<vector1, int>();
  static_assert(std::is_same<decltype(x0), std::vector<int>>::value, "");
  static_assert(std::is_same<decltype(x0a), std::vector<int>>::value, "");
  EXPECT_TRUE(x0.empty());
  EXPECT_TRUE(x0a.empty());

  auto x11 = mplus(x1);
  auto x12 = mplus(x1, x2);
  auto x13 = mplus(x1, x2, x1);
  auto x13a = mplus(x12, x1);
  auto x13b = mplus(x13, x0);
  EXPECT_EQ(x1, x11);
  EXPECT_EQ(2, x12.size());
  EXPECT_EQ(3, x13.size());
  EXPECT_EQ(3, x13a.size());
  EXPECT_EQ(x13, x13a);
  EXPECT_EQ(x13, x13b);
}
