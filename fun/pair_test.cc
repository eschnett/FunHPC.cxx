#include <fun/pair.hpp>

#include <gtest/gtest.h>

using namespace fun;

template <typename T> using pair1 = std::pair<char, T>;

TEST(fun_pair, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<pair1>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), pair1<int>>::value, "");
  EXPECT_EQ(0, rs.second);

  auto rs1 = iotaMap<pair1>([](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), pair1<double>>::value, "");
  EXPECT_EQ(-1, rs1.second);
}

TEST(fun_pair, fmap) {
  auto xs = std::pair<char, int>('a', 0);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_EQ(1, rs.second);

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_EQ(2, rs2.second);

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_EQ(1, rs3.second);

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_pair, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<pair1>([](auto x) { return int(x); }, s);
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

TEST(fun_pair, monad) {
  auto x1 = munit<pair1>(1);
  static_assert(std::is_same<decltype(x1), pair1<int>>::value, "");
  EXPECT_EQ(1, x1.second);

  auto xx1 = munit<pair1>(x1);
  EXPECT_EQ(1, xx1.second.second);

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind([](auto x, auto c) { return munit<pair1>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), pair1<int>>::value, "");
  EXPECT_EQ(2, x2.second);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));
  auto r2 =
      mfoldMap([](auto x) { return x; }, [](auto x, auto y) { return x + y; },
               0, std::make_pair('a', 1));
  EXPECT_EQ(std::make_pair('a', 1), r2);

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1.second));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
}
