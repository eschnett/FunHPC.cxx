#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <adt/nested.hpp>

#include <gtest/gtest.h>

#include <type_traits>

namespace {
template <typename T> using vector1 = std::vector<T>;
template <typename T> using nested1 = adt::nested<std::shared_ptr, vector1, T>;
}

TEST(adt_nested, iota) {
  auto xs = fun::iota<nested1>([](auto x) { return double(x); }, 10);
  static_assert(std::is_same<decltype(xs), nested1<double>>::value, "");
  EXPECT_TRUE(bool(xs.data));
  EXPECT_EQ(10, xs.data->size());
  EXPECT_EQ(5.0, xs.data->at(5));
}

TEST(adt_nested, fmap) {
  auto xs = fun::iota<nested1>([](auto x) { return double(x); }, 10);
  auto ys = fun::fmap([](auto x) { return x + 1.0; }, xs);
  EXPECT_TRUE(bool(ys.data));
  EXPECT_EQ(10, ys.data->size());
  EXPECT_EQ(6.0, ys.data->at(5));

  auto zs = fun::fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_TRUE(bool(zs.data));
  EXPECT_EQ(10, zs.data->size());
  EXPECT_EQ(11.0, zs.data->at(5));
}

TEST(adt_nested, foldMap) {
  auto xs = fun::iota<nested1>([](auto x) { return double(x); }, 10);

  auto r = fun::foldMap([](auto x) { return x; },
                        [](auto x, auto y) { return x + y; }, 0.0, xs);
  EXPECT_EQ(45.0, r);
}

TEST(adt_nested, monad) {
  auto x1 = fun::munit<nested1>(1);
  static_assert(std::is_same<decltype(x1), nested1<int>>::value, "");
  EXPECT_EQ(1, x1.data->size());
  EXPECT_EQ(1, x1.data->at(0));

  auto xx1 = fun::munit<nested1>(x1);
  EXPECT_EQ(1, xx1.data->size());
  EXPECT_EQ(1, xx1.data->at(0).data->size());
  EXPECT_EQ(1, xx1.data->at(0).data->at(0));

  auto x1j = fun::mjoin(xx1);
  EXPECT_EQ(*x1.data, *x1j.data);

  auto x2 = fun::mbind(
      [](auto x, auto c) { return fun::munit<nested1>(x + c); }, x1, 1);
  static_assert(std::is_same<decltype(x2), nested1<int>>::value, "");
  EXPECT_EQ(1, x2.data->size());
  EXPECT_EQ(2, x2.data->at(0));

  auto r = fun::mextract(x1);
  EXPECT_EQ(1, r);

  auto x0 = fun::mzero<nested1, int>();
  auto x0a = fun::mzero<nested1, int>();
  static_assert(std::is_same<decltype(x0), nested1<int>>::value, "");
  static_assert(std::is_same<decltype(x0a), nested1<int>>::value, "");
  EXPECT_TRUE(!bool(x0.data));
  EXPECT_TRUE(!bool(x0a.data));

  auto x11 = fun::mplus(x1);
  auto x12 = fun::mplus(x1, x2);
  auto x13 = fun::mplus(x1, x2, x1);
  auto x13a = fun::mplus(x12, x1);
  auto x13b = fun::mplus(x13, x0);
  EXPECT_EQ(*x1.data, *x11.data);
  EXPECT_EQ(2, x12.data->size());
  EXPECT_EQ(3, x13.data->size());
  EXPECT_EQ(3, x13a.data->size());
  EXPECT_EQ(*x13.data, *x13a.data);
  EXPECT_EQ(*x13.data, *x13b.data);

  EXPECT_FALSE(fun::mempty(x1));
  EXPECT_FALSE(fun::mempty(xx1));
  EXPECT_FALSE(fun::mempty(xx1.data->at(0)));
  EXPECT_FALSE(fun::mempty(x1j));
  EXPECT_FALSE(fun::mempty(x2));
  EXPECT_TRUE(fun::mempty(x0));
  EXPECT_TRUE(fun::mempty(x0a));
  EXPECT_FALSE(fun::mempty(x11));
  EXPECT_FALSE(fun::mempty(x12));
  EXPECT_FALSE(fun::mempty(x13));
  EXPECT_FALSE(fun::mempty(x13a));
  EXPECT_FALSE(fun::mempty(x13b));
}
