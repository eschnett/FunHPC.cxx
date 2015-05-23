#include <fun/maybe.hpp>

#include <gtest/gtest.h>

using namespace fun;

TEST(fun_maybe, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<adt::maybe<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), adt::maybe<int>>::value, "");
  EXPECT_TRUE(rs.just());
  EXPECT_EQ(0, rs.get_just());

  auto rs0 = iotaMap<adt::maybe<adt::dummy>>([](int x) { return x; }, 0);
  EXPECT_FALSE(rs0.just());

  auto rs1 = iotaMap<adt::maybe<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), adt::maybe<double>>::value, "");
  EXPECT_TRUE(rs1.just());
  EXPECT_EQ(-1, rs1.get_just());
}

TEST(fun_maybe, fmap) {
  auto xs = adt::make_just<int>(0);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(rs.just());
  EXPECT_EQ(1, rs.get_just());

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(rs2.just());
  EXPECT_EQ(2, rs2.get_just());

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(rs3.just());
  EXPECT_EQ(1, rs3.get_just());

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_maybe, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<adt::maybe<adt::dummy>>([](auto x) { return int(x); }, s);
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

TEST(fun_maybe, monad) {
  auto x1 = munit<adt::maybe<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), adt::maybe<int>>::value, "");
  EXPECT_TRUE(x1.just());
  EXPECT_EQ(1, x1.get_just());

  auto xx1 = munit<adt::maybe<adt::dummy>>(x1);
  EXPECT_TRUE(xx1.just());
  EXPECT_TRUE(xx1.get_just().just());
  EXPECT_EQ(1, xx1.get_just().get_just());

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind([](auto x, auto c) {
    return munit<adt::maybe<adt::dummy>>(x + c);
  }, x1, 1);
  static_assert(std::is_same<decltype(x2), adt::maybe<int>>::value, "");
  EXPECT_TRUE(x2.just());
  EXPECT_EQ(2, x2.get_just());

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));

  auto x0 = mzero<adt::maybe<adt::dummy>, int>();
  static_assert(std::is_same<decltype(x0), adt::maybe<int>>::value, "");
  EXPECT_FALSE(x0.just());

  auto x11 = mplus(x1);
  auto x12 = mplus(x1, x2);
  auto x13 = mplus(x1, x2, x1);
  auto x13a = mplus(x12, x1);
  auto x13b = mplus(x13, x0);
  EXPECT_EQ(x1, x11);
  EXPECT_TRUE(x12.just());
  EXPECT_TRUE(x13.just());
  EXPECT_TRUE(x13a.just());
  EXPECT_EQ(x13, x13a);
  EXPECT_EQ(x13, x13b);

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1.get_just()));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
  EXPECT_FALSE(mempty(x11));
  EXPECT_FALSE(mempty(x12));
  EXPECT_FALSE(mempty(x13));
  EXPECT_FALSE(mempty(x13a));
  EXPECT_FALSE(mempty(x13b));
}
