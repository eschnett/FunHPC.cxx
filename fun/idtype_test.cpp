#include <fun/idtype.hpp>

#include <gtest/gtest.h>

using namespace fun;

TEST(fun_idtype, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<adt::idtype<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), adt::idtype<int>>::value, "");
  EXPECT_EQ(0, rs.get());

  auto rs1 = iotaMap<adt::idtype<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), adt::idtype<double>>::value, "");
  EXPECT_EQ(-1, rs1.get());
}

TEST(fun_idtype, dump) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<adt::idtype<adt::dummy>>([](int x) { return x; }, s);
  std::string str(dump(rs));
  EXPECT_EQ("idtype{0,}", str);
}

TEST(fun_idtype, fmap) {
  auto xs = adt::idtype<int>(0);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_EQ(1, rs.get());

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_EQ(2, rs2.get());

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_EQ(1, rs3.get());

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_idtype, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<adt::idtype<adt::dummy>>([](auto x) { return int(x); }, s);
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

TEST(fun_idtype, monad) {
  auto x1 = munit<adt::idtype<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), adt::idtype<int>>::value, "");
  EXPECT_EQ(1, x1.get());

  auto xx1 = munit<adt::idtype<adt::dummy>>(x1);
  EXPECT_EQ(1, xx1.get().get());

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1, x1j);

  auto x2 = mbind(
      [](auto x, auto c) { return munit<adt::idtype<adt::dummy>>(x + c); }, x1,
      1);
  static_assert(std::is_same<decltype(x2), adt::idtype<int>>::value, "");
  EXPECT_EQ(2, x2.get());

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1.get()));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
}
