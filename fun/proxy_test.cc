#include <fun/proxy.hpp>

#include <gtest/gtest.h>

#include <atomic>

using namespace fun;

TEST(fun_proxy, iotaMap) {
  std::ptrdiff_t s = 1;
  auto rs = iotaMap<funhpc::proxy<adt::dummy>>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), funhpc::proxy<int>>::value, "");
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(0, *rs);

  auto rs1 = iotaMap<funhpc::proxy<adt::dummy>>(
      [](int x, int y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), funhpc::proxy<double>>::value, "");
  EXPECT_TRUE(bool(rs1));
  EXPECT_EQ(-1, *rs1);
}

namespace {
auto add1(int i) { return i + 1; }

auto add(int i, int j) { return i + j; }

auto accumulate(int i, funhpc::rptr<std::atomic<int>> accum) {
  return *accum += i;
}
auto call_accumulate(int i, funhpc::rptr<std::atomic<int>> accum) {
  return funhpc::async(funhpc::rlaunch::sync, accum.get_proc(), accumulate, i,
                       accum).get();
}
}

TEST(fun_proxy, fmap) {
  auto xs = funhpc::make_local_proxy<int>(0);

  auto rs = fmap(add1, xs);
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(1, *rs);

  auto rs2 = fmap(add, xs, 2);
  EXPECT_TRUE(bool(rs2));
  EXPECT_EQ(2, *rs2);

  auto rs3 = fmap2(add, xs, rs);
  EXPECT_TRUE(bool(rs3));
  EXPECT_EQ(1, *rs3);

  std::atomic<int> accum{0};
  fmap(call_accumulate, xs, funhpc::rptr<std::atomic<int>>(&accum));
  EXPECT_EQ(0, accum);
}

namespace {
int id(int x) { return x; }
int sq(int x) { return x * x; }
}

TEST(fun_proxy, foldMap) {
  std::ptrdiff_t s = 1;
  auto xs = iotaMap<funhpc::proxy<adt::dummy>>(id, s);
  auto ys = xs;

  auto sum = foldMap(id, add, 0, xs);
  static_assert(std::is_same<decltype(sum), int>::value, "");
  EXPECT_EQ((s - 1) * s / 2, sum);

  auto sum2 = foldMap2(add, add, 0, xs, ys);
  static_assert(std::is_same<decltype(sum2), int>::value, "");
  EXPECT_EQ((s - 1) * s, sum2);

  auto sum_sq = foldMap(sq, add, 0, xs);
  static_assert(std::is_same<decltype(sum), int>::value, "");
  EXPECT_EQ((s - 1) * s * (2 * s - 1) / 6, sum_sq);
}

namespace {
auto mkproxy_add(int x, int y) {
  return munit<funhpc::proxy<adt::dummy>>(x + y);
}
}

TEST(fun_proxy, monad) {
  auto x1 = munit<funhpc::proxy<adt::dummy>>(1);
  static_assert(std::is_same<decltype(x1), funhpc::proxy<int>>::value, "");
  EXPECT_TRUE(bool(x1));
  EXPECT_EQ(1, *x1);

  auto xx1 = munit<funhpc::proxy<adt::dummy>>(x1);
  EXPECT_TRUE(bool(xx1));
  EXPECT_TRUE(bool(*xx1));
  EXPECT_EQ(1, **xx1);

  auto x1j = mjoin(xx1);
  EXPECT_EQ(*x1, *x1j);

  auto x2 = mbind(mkproxy_add, x1, 1);
  static_assert(std::is_same<decltype(x2), funhpc::proxy<int>>::value, "");
  EXPECT_TRUE(bool(x2));
  EXPECT_EQ(2, *x2);

  auto r = mextract(x1);
  EXPECT_EQ(1, r);

  auto r1 = mfoldMap(id, add, 0, x1);
  EXPECT_EQ(r, mextract(r1));
}
