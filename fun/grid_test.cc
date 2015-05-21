#include <adt/array.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/topology.hpp>
#include <fun/vector.hpp>

#include <fun/nested.hpp>
#include <fun/grid.hpp>

#include <gtest/gtest.h>

using namespace fun;

namespace {
template <typename T> using std_vector = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, std_vector, T>;
// TODO: test shared_vector as well, once it has been implemented
template <typename T> using grid0 = adt::grid<std_vector, T, 0>;
template <typename T> using grid1 = adt::grid<std_vector, T, 1>;
template <typename T> using grid2 = adt::grid<std_vector, T, 2>;
template <typename T> using grid3 = adt::grid<std_vector, T, 3>;
}

TEST(fun_grid, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMap<grid3>([](const typename grid3<int>::index_type &i) {
    return int(adt::sum(i));
  }, typename grid3<int>::index_type{{s, s, s}});
  static_assert(std::is_same<decltype(rs), grid3<int>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());

  auto rs1 = iotaMap<grid3>([](const typename grid3<double>::index_type &i) {
    return double(adt::sum(i));
  }, typename grid3<double>::index_type{{s, s}});
  static_assert(std::is_same<decltype(rs1), grid3<double>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());
}

TEST(fun_grid, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<grid3>([](const typename grid3<int>::index_type &x) {
    return int(adt::sum(x));
  }, typename grid3<int>::index_type{{s, s, s}});
  EXPECT_EQ(27, xs.last());

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);
  EXPECT_EQ(28, ys.last());

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_EQ(55, zs.last());
}

TEST(fun_grid, boundary) {
  std::ptrdiff_t s = 10;

  auto xs1 = iotaMap<grid1>([](const typename grid1<int>::index_type &x) {
    return int(adt::sum(x * x));
  }, typename grid1<int>::index_type{{s}});
  typedef fun::fun_traits<grid1<int>>::template boundary_constructor<int>
      bgrid1;
  std::array<bgrid1, 2> bxs1;
  for (std::ptrdiff_t i = 0; i < 2; ++i) {
    bxs1[i] = fun::boundary(xs1, i);
    EXPECT_EQ(fun::msize(bxs1[i]), 1);
  }
  EXPECT_EQ(fun::mextract(bxs1[0]), 0);
  EXPECT_EQ(fun::mextract(bxs1[1]), (s - 1) * (s - 1));

#warning "TODO: test higher dimensions"
}

#warning "TODO: test fmapBoundary"

TEST(fun_grid, fmapStencil) {
  std::ptrdiff_t s = 10;

  auto xs0 = iotaMap<grid0>([](const typename grid0<int>::index_type &x) {
    return int(adt::sum(x * x));
  }, typename grid0<int>::index_type{{}});
  auto ys0 = fmapStencil([](auto x, auto bdirs) { return 0; },
                         [](auto x, auto i) { return x; }, xs0);
  auto sum0 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys0);
  EXPECT_EQ(0, sum0);

  auto xs1 = iotaMap<grid1>([](const typename grid1<int>::index_type &x) {
    return int(adt::sum(x * x));
  }, typename grid1<int>::index_type{{s}});
  auto bms1 = iotaMap<grid0>([](const typename grid0<int>::index_type &x) {
    return int(-1 * -1);
  }, typename grid0<int>::index_type{{}});
  auto bps1 = iotaMap<grid0>([s](const typename grid0<int>::index_type &x) {
    return int(s * s);
  }, typename grid0<int>::index_type{{}});
  auto ys1 = fmapStencil(
      [](auto x, auto bdirs, auto bm0, auto bp0) { return bm0 - 2 * x + bp0; },
      [](auto x, auto i) { return x; }, xs1, bms1, bps1);
  auto sum1 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys1);
  EXPECT_EQ(20, sum1);

  auto xs2 = iotaMap<grid2>([](const typename grid2<int>::index_type &x) {
    return int(adt::sum(x * x));
  }, typename grid2<int>::index_type{{s, s}});
  auto bms2 = iotaMap<grid1>([](const typename grid1<int>::index_type &x) {
    return int(-1 * -1 + adt::sum(x * x));
  }, typename grid1<int>::index_type{{s}});
  auto bps2 = iotaMap<grid1>([s](const typename grid1<int>::index_type &x) {
    return int(s * s + adt::sum(x * x));
  }, typename grid1<int>::index_type{{s}});
  auto ys2 = fmapStencil(
      [](auto x, auto bdirs, auto bm0, auto bm1, auto bp0, auto bp1) {
        return (bm0 - 2 * x + bp0) + (bm1 - 2 * x + bp1);
      },
      [](auto x, auto i) { return x; }, xs2, bms2, bms2, bps2, bps2);
  auto sum2 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys2);
  EXPECT_EQ(400, sum2);
}

TEST(fun_grid, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<grid3>([](const typename grid3<int>::index_type &x) {
    return int(adt::sum(x));
  }, typename grid3<int>::index_type{{s, s, s}});
  EXPECT_EQ(27, xs.last());
  auto r = foldMap([](auto x) { return x; },
                   [](auto x, auto y) { return x + y; }, 0, xs);
  EXPECT_EQ(13500, r);
}

TEST(fun_grid, monad) {
  auto xs = munit<grid3>(1);
  auto xss = munit<grid3>(xs);
  //   auto ys = mjoin(xss);
  //   auto zs = mbind([](auto x) { return munit<grid3>(x); }, ys);
  //   auto z = mextract(zs);
  //   EXPECT_EQ(1, z);

  // auto z1 = mfoldMap([](auto x) { return x; },
  //                    [](auto x, auto y) { return x + y; }, 0, zs);
  // EXPECT_EQ(1, z1.size());
  // EXPECT_EQ(z, mextract(z1));

  auto ns = mzero<grid3, int>();
  EXPECT_TRUE(ns.empty());
  //   auto ps = mplus(ns, xs, ys, zs);
  //   EXPECT_EQ(3, ps.size());
  //   auto ss = msome<grid3>(1, 2, 3);
  //   EXPECT_EQ(3, ss.size());
  EXPECT_TRUE(mempty(ns));
  //   EXPECT_FALSE(mempty(ps));
  //   EXPECT_FALSE(mempty(ss));
}
