#include <fun/either.hpp>

#include <gtest/gtest.h>

using namespace fun;

template <typename T> using either1 = adt::either<char, T>;

TEST(fun_either, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<either1<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), either1<int>>::value, "");
  EXPECT_TRUE(rs.right());
  EXPECT_EQ(0, rs.get_right());

  auto rs0 = iotaMap<either1<adt::dummy>>([](int x) { return x; }, 0);
  EXPECT_FALSE(rs0.right());

  auto rs1 = iotaMap<either1<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), either1<double>>::value, "");
  EXPECT_TRUE(rs1.right());
  EXPECT_EQ(-1, rs1.get_right());
}

TEST(fun_either, fmap) {
  auto xs = adt::make_right<char, int>(0);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(rs.right());
  EXPECT_EQ(1, rs.get_right());

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(rs2.right());
  EXPECT_EQ(2, rs2.get_right());

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(rs3.right());
  EXPECT_EQ(1, rs3.get_right());

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_either, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<either1<adt::dummy>>([](auto x) { return int(x); }, s);
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

TEST(fun_either, monad) {
  auto x1 = munit<either1<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), either1<int>>::value, "");
  EXPECT_TRUE(x1.right());
  EXPECT_EQ(1, x1.get_right());

  auto xx1 = munit<either1<adt::dummy>>(x1);
  EXPECT_TRUE(xx1.right());
  EXPECT_TRUE(xx1.get_right().right());
  EXPECT_EQ(1, xx1.get_right().get_right());

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind(
      [](auto x, auto c) { return munit<either1<adt::dummy>>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), either1<int>>::value, "");
  EXPECT_TRUE(x2.right());
  EXPECT_EQ(2, x2.get_right());

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));

  auto x0 = mzero<either1<adt::dummy>, int>();
  static_assert(std::is_same<decltype(x0), either1<int>>::value, "");
  EXPECT_FALSE(x0.right());

  auto x11 = mplus(x1);
  auto x12 = mplus(x1, x2);
  auto x13 = mplus(x1, x2, x1);
  auto x13a = mplus(x12, x1);
  auto x13b = mplus(x13, x0);
  EXPECT_EQ(x1, x11);
  EXPECT_TRUE(x12.right());
  EXPECT_TRUE(x13.right());
  EXPECT_TRUE(x13a.right());
  EXPECT_EQ(x13, x13a);
  EXPECT_EQ(x13, x13b);

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1.get_right()));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
  EXPECT_FALSE(mempty(x11));
  EXPECT_FALSE(mempty(x12));
  EXPECT_FALSE(mempty(x13));
  EXPECT_FALSE(mempty(x13a));
  EXPECT_FALSE(mempty(x13b));
}
