#include <fun/function.hpp>

#include <gtest/gtest.h>

using namespace fun;

template <typename R> using function1 = std::function<R(int)>;

TEST(fun_function, iota) {
  std::ptrdiff_t s = 1;
  auto rs = iota<function1>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), function1<int>>::value, "");
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(0, rs(42));

  auto rs0 = iota<function1>([](int x) { return x; }, 0);
  EXPECT_FALSE(bool(rs0));

  auto rs1 = iota<function1>([](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), function1<double>>::value, "");
  EXPECT_TRUE(bool(rs1));
  EXPECT_EQ(-1, rs1(42));
}

TEST(fun_function, fmap) {
  function1<int> xs = [](int) { return 0; };

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
  auto x1 = munit<function1>(1);
  static_assert(std::is_same<decltype(x1), function1<int>>::value, "");
  EXPECT_TRUE(bool(x1));
  EXPECT_EQ(1, x1(42));

  auto xx1 = munit<function1>(x1);
  EXPECT_TRUE(bool(xx1));
  EXPECT_TRUE(bool(xx1(42)));
  EXPECT_EQ(1, xx1(42)(42));

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1(42), x1j(42));

  auto x2 =
      mbind([](auto x, auto c) { return munit<function1>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), function1<int>>::value, "");
  EXPECT_TRUE(bool(x2));
  EXPECT_EQ(2, x2(42));

  auto x0 = mzero<function1, int>();
  static_assert(std::is_same<decltype(x0), function1<int>>::value, "");
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
