#include <fun/shared_ptr.hpp>

#include <gtest/gtest.h>

using namespace fun;

TEST(fun_shared_ptr, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<std::shared_ptr<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), std::shared_ptr<int>>::value, "");
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(0, *rs);

  auto rs0 = iotaMap<std::shared_ptr<adt::dummy>>([](int x) { return x; }, 0);
  EXPECT_FALSE(bool(rs0));

  auto rs1 = iotaMap<std::shared_ptr<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), std::shared_ptr<double>>::value,
                "");
  EXPECT_TRUE(bool(rs1));
  EXPECT_EQ(-1, *rs1);
}

TEST(fun_shared_ptr, dump) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<std::shared_ptr<adt::dummy>>([](int x) { return x; }, s);
  std::string str(dump(rs));
  EXPECT_EQ("shared_ptr{0}", str);

  auto rs0 = iotaMap<std::shared_ptr<adt::dummy>>([](int x) { return x; }, 0);
  std::string str0(dump(rs0));
  EXPECT_EQ("shared_ptr{}", str0);
}

TEST(fun_shared_ptr, fmap) {
  auto xs = std::make_shared<int>(0);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(1, *rs);

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(bool(rs2));
  EXPECT_EQ(2, *rs2);

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(bool(rs3));
  EXPECT_EQ(1, *rs3);

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_shared_ptr, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs =
      iotaMap<std::shared_ptr<adt::dummy>>([](auto x) { return int(x); }, s);
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

TEST(fun_shared_ptr, monad) {
  auto x1 = munit<std::shared_ptr<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), std::shared_ptr<int>>::value, "");
  EXPECT_TRUE(bool(x1));
  EXPECT_EQ(1, *x1);

  auto xx1 = munit<std::shared_ptr<adt::dummy>>(x1);
  EXPECT_TRUE(bool(xx1));
  EXPECT_TRUE(bool(*xx1));
  EXPECT_EQ(1, **xx1);

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind(
      [](auto x, auto c) { return munit<std::shared_ptr<adt::dummy>>(x + c); },
      x1, 1);
  static_assert(std::is_same<decltype(x2), std::shared_ptr<int>>::value, "");
  EXPECT_TRUE(bool(x2));
  EXPECT_EQ(2, *x2);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));

  auto x0 = mzero<std::shared_ptr<adt::dummy>, int>();
  static_assert(std::is_same<decltype(x0), std::shared_ptr<int>>::value, "");
  EXPECT_FALSE(bool(x0));

  auto x11 = mplus(x1);
  auto x12 = mplus(x1, x2);
  auto x13 = mplus(x1, x2, x1);
  auto x13a = mplus(x12, x1);
  auto x13b = mplus(x13, x0);
  EXPECT_EQ(x1, x11);
  EXPECT_TRUE(bool(x12));
  EXPECT_TRUE(bool(x13));
  EXPECT_TRUE(bool(x13a));
  EXPECT_EQ(x13, x13a);
  EXPECT_EQ(x13, x13b);

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(*xx1));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
  EXPECT_FALSE(mempty(x11));
  EXPECT_FALSE(mempty(x12));
  EXPECT_FALSE(mempty(x13));
  EXPECT_FALSE(mempty(x13a));
  EXPECT_FALSE(mempty(x13b));
}
