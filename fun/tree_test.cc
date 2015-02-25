#include <fun/shared_ptr.hpp>
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
template <typename T> using tree1 = adt::tree<shared_vector, T>;
}

TEST(fun_tree, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<tree1>([](auto x) { return int(x); }, s);
  static_assert(std::is_same<decltype(rs), tree1<int>>::value, "");
  EXPECT_EQ(s, rs.size());

  auto rs1 =
      iotaMap<tree1>([](auto x, auto y) { return double(x + y); }, s, -1);
  static_assert(std::is_same<decltype(rs1), tree1<double>>::value, "");
}

TEST(fun_tree, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<tree1>([](auto x) { return int(x); }, s);

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
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
