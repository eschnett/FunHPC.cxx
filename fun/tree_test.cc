#include <fun/shared_future.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/topology.hpp>
#include <fun/vector.hpp>

#include <adt/nested.hpp>
#include <fun/nested.hpp>

#include <adt/tree.hpp>
#include <fun/tree.hpp>

#include <gtest/gtest.h>

using namespace fun;

namespace {
template <typename T> using vector1 = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, vector1, T>;
template <typename T>
using future_vector = adt::nested<qthread::shared_future, vector1, T>;
template <typename T> using tree1 = adt::tree<shared_vector, T>;
template <typename T> using tree2 = adt::tree<future_vector, T>;
}

TEST(fun_tree, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<tree1>([](auto x) { return int(x); }, s);
  static_assert(std::is_same<decltype(rs), tree1<int>>::value, "");
  EXPECT_EQ(s, rs.size());
  EXPECT_EQ(9, rs.last());

  auto rs1 =
      iotaMap<tree1>([](auto x, auto y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), tree1<double>>::value, "");
  EXPECT_EQ(8, rs1.last());
}

TEST(fun_tree, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<tree1>([](auto x) { return int(x); }, s);
  EXPECT_EQ(9, xs.last());

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);
  EXPECT_EQ(10, ys.last());

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_EQ(19, zs.last());
}

TEST(fun_tree, fmapTopo) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<tree1>([](int x) { return x * x; }, s);
  auto ys = fmapTopo(
      [](auto x, const auto &bs) { return get<0>(bs) - 2 * x + get<1>(bs); },
      [](auto x, auto i) { return x; }, xs, connectivity<int>(1, 100));
  auto sum = foldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, ys);
  EXPECT_EQ(20, sum);

  auto x2s = iotaMap<tree2>([](int x) { return x * x; }, s);
  // EXPECT_FALSE(x2s.data.ready());
  auto y2s = fmapTopo(
      [](auto x, const auto &bs) { return get<0>(bs) - 2 * x + get<1>(bs); },
      [](auto x, auto i) { return x; }, x2s, connectivity<int>(1, 100));
  // EXPECT_FALSE(x2s.data.ready());
  // EXPECT_FALSE(y2s.data.ready());
  auto sum2 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, y2s);
  // EXPECT_TRUE(x2s.data.ready());
  // EXPECT_TRUE(y2s.data.ready());
  EXPECT_EQ(20, sum2);
}

TEST(fun_tree, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<tree1>([](auto x) { return int(x); }, s);
  auto r = foldMap([](auto x) { return x; },
                   [](auto x, auto y) { return x + y; }, 0, xs);
  EXPECT_EQ(s * (s - 1) / 2, r);
}

TEST(fun_tree, monad) {
  auto xs = munit<tree1>(1);
  auto xss = munit<tree1>(xs);
  auto ys = mjoin(xss);
  auto zs = mbind([](auto x) { return munit<tree1>(x); }, ys);
  auto z = mextract(zs);
  EXPECT_EQ(1, z);
  auto ns = mzero<tree1, int>();
  EXPECT_TRUE(ns.empty());
  auto ps = mplus(ns, xs, ys, zs);
  EXPECT_EQ(3, ps.size());
  auto ss = msome<tree1>(1, 2, 3);
  EXPECT_EQ(3, ss.size());
  EXPECT_TRUE(mempty(ns));
  EXPECT_FALSE(mempty(ps));
  EXPECT_FALSE(mempty(ss));
}
