#include <fun/shared_future.hpp>

#include <gtest/gtest.h>
#include <qthread.h>

using namespace fun;

TEST(fun_shared_future, iotaMap) {
  qthread_initialize();

  std::ptrdiff_t s = 1;
  auto rs =
      iotaMap<qthread::shared_future<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(rs.valid());
  // EXPECT_FALSE(rs.ready());
  EXPECT_EQ(0, rs.get());

  auto rs1 = iotaMap<qthread::shared_future<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(
      std::is_same<decltype(rs1), qthread::shared_future<double>>::value, "");
  EXPECT_TRUE(rs1.valid());
  EXPECT_EQ(-1, rs1.get());
}

TEST(fun_shared_future, dump) {
  std::ptrdiff_t s = 1;
  auto rs =
      iotaMap<qthread::shared_future<adt::dummy>>([](int x) { return x; }, s);
  std::string str(dump(rs));
  EXPECT_EQ("shared_future{0}", str);

  auto rs0 = qthread::shared_future<int>();
  std::string str0(dump(rs0));
  EXPECT_EQ("shared_future{}", str0);
}

TEST(fun_shared_future, fmap) {
  auto xs =
      iotaMap<qthread::shared_future<adt::dummy>>([](int x) { return x; }, 1);

  auto rs = fmap([](int i) { return i + 1; }, xs);
  EXPECT_TRUE(rs.valid());
  // EXPECT_FALSE(xs.ready());
  // EXPECT_FALSE(rs.ready());

  auto rs2 = fmap([](int i, int j) { return i + j; }, xs, 2);
  EXPECT_TRUE(rs2.valid());
  // EXPECT_FALSE(xs.ready());
  // EXPECT_FALSE(rs2.ready());

  EXPECT_EQ(1, rs.get());
  EXPECT_EQ(2, rs2.get());

  auto rs3 = fmap2([](int i, int j) { return i + j; }, xs, rs);
  EXPECT_TRUE(rs3.valid());
  // EXPECT_FALSE(rs3.ready());
  EXPECT_EQ(1, rs3.get());

  int accum = 0;
  fmap([](int i, int *accum) { return *accum += i; }, xs, &accum);
  EXPECT_EQ(0, accum);
}

TEST(fun_shared_future, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<qthread::shared_future<adt::dummy>>(
      [](auto x) { return int(x); }, s);
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
  auto x1 = munit<qthread::shared_future<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(x1.valid());
  EXPECT_EQ(1, x1.get());

  auto xx1 = munit<qthread::shared_future<adt::dummy>>(x1);
  EXPECT_TRUE(xx1.valid());
  EXPECT_TRUE(xx1.get().valid());
  EXPECT_EQ(1, xx1.get().get());

  auto x1j = mjoin(xx1);
  EXPECT_EQ(x1.get(), x1j.get());

  auto x2 = mbind(
      [](auto x, auto c) {
        return munit<qthread::shared_future<adt::dummy>>(x + c);
      },
      x1, 1);
  static_assert(std::is_same<decltype(x2), qthread::shared_future<int>>::value,
                "");
  EXPECT_TRUE(x2.valid());
  EXPECT_EQ(2, x2.get());

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, x1);
  EXPECT_EQ(r, mextract(r1));
}
