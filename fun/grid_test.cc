#include <adt/array.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/topology.hpp>
#include <fun/vector.hpp>

#include <adt/nested.hpp>
#include <fun/nested.hpp>

#include <adt/grid.hpp>
#include <fun/grid.hpp>

#include <gtest/gtest.h>

using namespace fun;

namespace {
template <typename T> using vector1 = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, vector1, T>;
template <typename T> using grid1 = adt::grid<vector1, T, 3>;
template <typename T> using grid2 = adt::grid<shared_vector, T, 2>;
}

TEST(fun_grid, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<grid1>(
      [](const typename grid1<int>::index_type &i) { return int(adt::sum(i)); },
      typename grid1<int>::index_type{{s, s, s}});
  static_assert(std::is_same<decltype(rs), grid1<int>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());

  auto rs1 = iotaMap<grid1>([](const typename grid1<double>::index_type &i) {
                              return double(adt::sum(i));
                            },
                            typename grid1<double>::index_type{{s, s}});
  static_assert(std::is_same<decltype(rs1), grid1<double>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());
}

TEST(fun_grid, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<grid1>(
      [](const typename grid1<int>::index_type &x) { return int(adt::sum(x)); },
      typename grid1<int>::index_type{{s, s, s}});
  EXPECT_EQ(27, xs.last());

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);
  EXPECT_EQ(28, ys.last());

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_EQ(55, zs.last());
}

// TEST(fun_grid, fmapTopo) {
//   std::ptrdiff_t s = 10;
//   auto xs = iotaMap<grid1>([](const typename grid1<int>::index_type & x) {
//   return x * x; }, typename grid1<int>::index_type{{s, s, s}});
//   auto ys = fmapTopo(
//       [](auto x, const auto &bs) { return get<0>(bs) - 2 * x + get<1>(bs); },
//       [](auto x, auto i) { return x; }, xs, connectivity<int>(1, 100));
//   auto sum = foldMap([](auto x) { return x; },
//                      [](auto x, auto y) { return x + y; }, 0, ys);
//   EXPECT_EQ(20, sum);
//
//   auto x2s = iotaMap<grid2>([](int x) { return x * x; }, s);
//   // EXPECT_FALSE(x2s.data.ready());
//   auto y2s = fmapTopo(
//       [](auto x, const auto &bs) { return get<0>(bs) - 2 * x + get<1>(bs); },
//       [](auto x, auto i) { return x; }, x2s, connectivity<int>(1, 100));
//   // EXPECT_FALSE(x2s.data.ready());
//   // EXPECT_FALSE(y2s.data.ready());
//   auto sum2 = foldMap([](auto x) { return x; },
//                       [](auto x, auto y) { return x + y; }, 0, y2s);
//   // EXPECT_TRUE(x2s.data.ready());
//   // EXPECT_TRUE(y2s.data.ready());
//   EXPECT_EQ(20, sum2);
// }

TEST(fun_grid, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<grid1>(
      [](const typename grid1<int>::index_type &x) { return int(adt::sum(x)); },
      typename grid1<int>::index_type{{s, s, s}});
  EXPECT_EQ(27, xs.last());
  auto r = foldMap([](auto x) { return x; },
                   [](auto x, auto y) { return x + y; }, 0, xs);
  EXPECT_EQ(13500, r);
}

TEST(fun_grid, monad) {
  auto xs = munit<grid1>(1);
  auto xss = munit<grid1>(xs);
  //   auto ys = mjoin(xss);
  //   auto zs = mbind([](auto x) { return munit<grid1>(x); }, ys);
  //   auto z = mextract(zs);
  //   EXPECT_EQ(1, z);

  // auto z1 = mfoldMap([](auto x) { return x; },
  //                    [](auto x, auto y) { return x + y; }, 0, zs);
  // EXPECT_EQ(1, z1.size());
  // EXPECT_EQ(z, mextract(z1));

  auto ns = mzero<grid1, int>();
  EXPECT_TRUE(ns.empty());
  //   auto ps = mplus(ns, xs, ys, zs);
  //   EXPECT_EQ(3, ps.size());
  //   auto ss = msome<grid1>(1, 2, 3);
  //   EXPECT_EQ(3, ss.size());
  EXPECT_TRUE(mempty(ns));
  //   EXPECT_FALSE(mempty(ps));
  //   EXPECT_FALSE(mempty(ss));
}
