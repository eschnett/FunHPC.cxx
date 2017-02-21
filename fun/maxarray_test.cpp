#include <fun/maxarray.hpp>

#include <fun/fun_decl.hpp>
#include <fun/idtype.hpp>

#include <fun/fun_impl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>

using namespace fun;

constexpr std::size_t max_size = 100;
template <typename T> using adt_maxarray = adt::maxarray<T, max_size>;

TEST(fun_maxarray, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<adt_maxarray<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), adt_maxarray<int>>::value, "");
  EXPECT_EQ(s, rs.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i, rs[i]);

  auto rs1 = iotaMap<adt_maxarray<adt::dummy>>(
      [](auto x, auto y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), adt_maxarray<double>>::value, "");
  EXPECT_EQ(s, rs1.size());
  for (std::ptrdiff_t i = 0; i < s; ++i)
    EXPECT_EQ(i - 1, rs1[i]);
}

TEST(fun_maxarray, dump) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<adt_maxarray<adt::dummy>>([](int x) { return x; }, s);
  std::string str(dump(rs));
  EXPECT_EQ("maxarray{0,1,2,3,4,5,6,7,8,9,}", str);
}

TEST(fun_maxarray, fmap) {
  std::ptrdiff_t s = 10;
  adt_maxarray<int> xs(s);
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

TEST(fun_maxarray, fmapStencil) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<adt_maxarray<adt::dummy>>([](int x) { return x * x; }, s);

  auto ys = fmapStencil(
      [](auto x, auto bdirs, auto bm, auto bp) { return bm - 2 * x + bp; },
      [](auto x, auto i) { return x; }, xs, 0b11, 1, 100);
  auto ysum = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys);
  EXPECT_EQ(20, ysum);

  auto zs = fmapStencilMulti<1>(
      [](auto x, auto bdirs, auto bm, auto bp) { return bm - 2 * x + bp; },
      [](auto x, auto i) { return x; }, xs, 0b11, adt::idtype<int>(1),
      adt::idtype<int>(100));
  auto zsum = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, zs);
  EXPECT_EQ(20, zsum);
}

TEST(fun_maxarray, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<adt_maxarray<adt::dummy>>([](auto x) { return int(x); }, s);
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

TEST(fun_maxarray, monad) {
  auto x1 = munit<adt_maxarray<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), adt_maxarray<int>>::value, "");
  EXPECT_EQ(1, x1.size());
  EXPECT_EQ(1, x1[0]);

  auto xx1 = munit<adt_maxarray<adt::dummy>>(x1);
  EXPECT_EQ(1, xx1.size());
  EXPECT_EQ(1, xx1[0].size());
  EXPECT_EQ(1, xx1[0][0]);

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind(
      [](auto x, auto c) { return munit<adt_maxarray<adt::dummy>>(x + c); }, x1,
      1);
  static_assert(std::is_same<decltype(x2), adt_maxarray<int>>::value, "");
  EXPECT_EQ(1, x2.size());
  EXPECT_EQ(2, x2[0]);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  static_assert(std::is_same<decltype(r1), adt_maxarray<int>>::value, "");
  EXPECT_EQ(1, r1.size());
  EXPECT_EQ(r, mextract(r1));

  auto x0 = mzero<adt_maxarray<adt::dummy>, int>();
  static_assert(std::is_same<decltype(x0), adt_maxarray<int>>::value, "");
  EXPECT_TRUE(x0.empty());

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

  auto y1 = msome<adt_maxarray<adt::dummy>>(2);
  EXPECT_EQ(1, y1.size());

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1[0]));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
  EXPECT_FALSE(mempty(x11));
  EXPECT_FALSE(mempty(x12));
  EXPECT_FALSE(mempty(x13));
  EXPECT_FALSE(mempty(x13a));
  EXPECT_FALSE(mempty(x13b));
}
