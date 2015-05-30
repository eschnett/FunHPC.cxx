#include <fun/function.hpp>

#include <gtest/gtest.h>

using namespace fun;

template <typename R> using std_function = std::function<R(int)>;

TEST(fun_function, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<std_function<adt::dummy>>([](int x) { return x; },
                                              adt::irange_t(s));
  static_assert(std::is_same<decltype(rs), std_function<int>>::value, "");
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(0, rs(42));

  auto rs0 = iotaMap<std_function<adt::dummy>>([](int x) { return x; },
                                               adt::irange_t(0));
  EXPECT_FALSE(bool(rs0));

  auto rs1 = iotaMap<std_function<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, adt::irange_t(s), -1);
  static_assert(std::is_same<decltype(rs1), std_function<double>>::value, "");
  EXPECT_TRUE(bool(rs1));
  EXPECT_EQ(-1, rs1(42));
}

TEST(fun_function, fmap) {
  std_function<int> xs = [](int) { return 0; };

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(1, rs(42));

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(bool(rs2));
  EXPECT_EQ(2, rs2(42));

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(bool(rs3));
  EXPECT_EQ(1, rs3(42));

  int accum = 0;
  fmap([](int i, int *accum) { return *accum += i; }, xs, &accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_function, monad) {
  auto x1 = munit<std_function<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), std_function<int>>::value, "");
  EXPECT_TRUE(bool(x1));
  EXPECT_EQ(1, x1(42));

  auto xx1 = munit<std_function<adt::dummy>>(x1);
  EXPECT_TRUE(bool(xx1));
  EXPECT_TRUE(bool(xx1(42)));
  EXPECT_EQ(1, xx1(42)(42));

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1(42), x1j(42));

  auto x2 = mbind([](auto x, auto c) {
    return munit<std_function<adt::dummy>>(x + c);
  }, x1, 1);
  static_assert(std::is_same<decltype(x2), std_function<int>>::value, "");
  EXPECT_TRUE(bool(x2));
  EXPECT_EQ(2, x2(42));

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(1, r1(42));
  auto r2 =
      mfoldMap([](auto x) { return x; }, [](auto x, auto y) { return x + y; },
               0, std::function<int(int)>([](int x) { return x; }));
  EXPECT_EQ(42, r2(42));

  auto x0 = mzero<std_function<adt::dummy>, int>();
  static_assert(std::is_same<decltype(x0), std_function<int>>::value, "");
  EXPECT_FALSE(bool(x0));

  auto x11 = mplus(x1);
  auto x12 = mplus(x1, x2);
  auto x13 = mplus(x1, x2, x1);
  auto x13a = mplus(x12, x1);
  auto x13b = mplus(x13, x0);
  EXPECT_EQ(x1(42), x11(42));
  EXPECT_TRUE(bool(x12));
  EXPECT_TRUE(bool(x13));
  EXPECT_TRUE(bool(x13a));
  EXPECT_EQ(x13(42), x13a(42));
  EXPECT_EQ(x13(42), x13b(42));

  EXPECT_FALSE(mempty(x1));
  EXPECT_FALSE(mempty(xx1));
  EXPECT_FALSE(mempty(xx1(42)));
  EXPECT_FALSE(mempty(x1j));
  EXPECT_FALSE(mempty(x2));
  EXPECT_TRUE(mempty(x0));
  EXPECT_FALSE(mempty(x11));
  EXPECT_FALSE(mempty(x12));
  EXPECT_FALSE(mempty(x13));
  EXPECT_FALSE(mempty(x13a));
  EXPECT_FALSE(mempty(x13b));
}
