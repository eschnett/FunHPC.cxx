#include <fun/empty.hpp>

#include <gtest/gtest.h>

using namespace fun;

TEST(fun_empty, iotaMap) {
  std::ptrdiff_t s = 0;
  auto rs = iotaMap<adt::empty<adt::dummy>>([](int x) { return x; },
                                            adt::irange_t(s));
  static_assert(std::is_same<decltype(rs), adt::empty<int>>::value, "");

  auto rs1 = iotaMap<adt::empty<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, adt::irange_t(s), -1);
  static_assert(std::is_same<decltype(rs1), adt::empty<double>>::value, "");
}

TEST(fun_empty, fmap) {
  auto xs = adt::empty<int>();
  EXPECT_TRUE(mempty(xs));
  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(mempty(rs));
  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(mempty(rs2));
  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(mempty(rs3));
  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_empty, foldMap) {
  std::ptrdiff_t s = 0;
  auto xs = iotaMap<adt::empty<adt::dummy>>([](auto x) { return int(x); },
                                            adt::irange_t(s));
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
