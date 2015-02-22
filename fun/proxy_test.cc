#include <fun/proxy.hpp>

#include <gtest/gtest.h>

#include <atomic>

using namespace fun;

TEST(fun_proxy, iota) {
  std::size_t s = 1;
  auto rs = iota<funhpc::proxy>([](int x) { return x; }, s);
  static_assert(std::is_same<decltype(rs), funhpc::proxy<int>>::value, "");
  EXPECT_TRUE(bool(rs));
  EXPECT_EQ(0, *rs);

  auto rs1 =
      iota<funhpc::proxy>([](int x, int y) { return double(x + y); }, s, -1);
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

// TEST(fun_proxy, foldMap) {
//   std::size_t s = 1;
//   auto xs = iota<funhpc::proxy>([](auto x) { return x; }, s);
//   auto ys = xs;

//   auto sum = foldMap([](auto x) { return x; },
//                      [](auto x, auto y) { return x + y; }, 0, xs);
//   static_assert(std::is_same<decltype(sum), int>::value, "");
//   EXPECT_EQ((s - 1) * s / 2, sum);

//   auto sum2 = foldMap2([](auto x, auto y) { return x + y; },
//                        [](auto x, auto y) { return x + y; }, 0, xs, ys);
//   static_assert(std::is_same<decltype(sum2), int>::value, "");
//   EXPECT_EQ((s - 1) * s, sum2);

//   auto sum_sq = foldMap([](auto x) { return x * x; },
//                         [](auto x, auto y) { return x + y; }, 0, xs);
//   static_assert(std::is_same<decltype(sum), int>::value, "");
//   EXPECT_EQ((s - 1) * s * (2 * s - 1) / 6, sum_sq);
// }

// TEST(fun_proxy, monad) {
//   auto x1 = munit<funhpc::proxy>(1);
//   static_assert(std::is_same<decltype(x1), funhpc::proxy<int>>::value,
//                 "");
//   EXPECT_TRUE(x1.valid());
//   EXPECT_EQ(1, x1.get());
//
//   auto xx1 = munit<funhpc::proxy>(x1);
//   EXPECT_TRUE(xx1.valid());
//   EXPECT_TRUE(xx1.get().valid());
//   EXPECT_EQ(1, xx1.get().get());
//
//   auto x1j = mjoin(xx1);
//   EXPECT_EQ(x1.get(), x1j.get());
//
//   auto x2 =
//       mbind([](auto x, auto c) { return munit<funhpc::proxy>(x + c); },
//             x1, 1);
//   static_assert(std::is_same<decltype(x2), funhpc::proxy<int>>::value,
//                 "");
//   EXPECT_TRUE(x2.valid());
//   EXPECT_EQ(2, x2.get());
//
//   auto r = mextract(x1);
//   EXPECT_EQ(1, r);
// }
