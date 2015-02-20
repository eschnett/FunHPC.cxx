#include <fun/shared_future.hpp>

#include <gtest/gtest.h>
#include <qthread.h>

using namespace fun;

TEST(fun_shared_future, iota) {
  qthread_initialize();

  std::size_t s = 1;
  auto rs = iota<qthread::shared_future>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(rs.valid());
  EXPECT_EQ(0, rs.get());

  auto rs1 = iota<qthread::shared_future>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(
      std::is_same<decltype(rs1), qthread::shared_future<double>>::value, "");
  EXPECT_TRUE(rs1.valid());
  EXPECT_EQ(-1, rs1.get());
}

TEST(fun_shared_future, fmap) {
  auto xs = qthread::make_ready_future(0).share();

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(rs.valid());
  EXPECT_EQ(1, rs.get());

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(rs2.valid());
  EXPECT_EQ(2, rs2.get());

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(rs3.valid());
  EXPECT_EQ(1, rs3.get());

  int accum = 0;
  fmap([](int i, int &accum) { return accum += i; }, xs, accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_shared_future, foldMap) {
  std::size_t s = 1;
  auto xs = iota<qthread::shared_future>([](auto x) { return x; }, s);
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

TEST(fun_shared_future, monad) {
  auto x1 = munit<qthread::shared_future>(1);
  static_assert(std::is_same<decltype(x1), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(x1.valid());
  EXPECT_EQ(1, x1.get());

  auto xx1 = munit<qthread::shared_future>(x1);
  EXPECT_TRUE(xx1.valid());
  EXPECT_TRUE(xx1.get().valid());
  EXPECT_EQ(1, xx1.get().get());

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1.get(), x1j.get());

  auto x2 =
      mbind([](auto x, auto c) { return munit<qthread::shared_future>(x + c); },
            x1, 1);
  static_assert(std::is_same<decltype(x2), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(x2.valid());
  EXPECT_EQ(2, x2.get());

  auto r = mextract(x1);
  EXPECT_EQ(1, r);
}
