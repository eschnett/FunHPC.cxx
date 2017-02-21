#include <fun/grid_decl.hpp>

#include <fun/array.hpp>
#include <fun/idtype.hpp>
#include <fun/nested_decl.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <fun/grid_impl.hpp>

#include <fun/nested_impl.hpp>

#include <gtest/gtest.h>

using namespace fun;

namespace {
template <typename T>
using shared_vector =
    adt::nested<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;
// TODO: test shared_vector as well, once it has been implemented
template <typename T> using grid0 = adt::grid<std::vector<adt::dummy>, T, 0>;
template <typename T> using grid1 = adt::grid<std::vector<adt::dummy>, T, 1>;
template <typename T> using grid2 = adt::grid<std::vector<adt::dummy>, T, 2>;
template <typename T> using grid3 = adt::grid<std::vector<adt::dummy>, T, 3>;
}

TEST(fun_grid, iotaMap) {
  std::ptrdiff_t s = 10;
  auto rs = iotaMapMulti<grid3<adt::dummy>>(
      [](const auto &i) { return int(adt::sum(i)); },
      adt::steprange_t<3>(adt::index_t<3>{{s, s, s}}));
  static_assert(std::is_same<decltype(rs), grid3<int>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());

  auto rs1 = iotaMapMulti<grid3<adt::dummy>>(
      [](const typename grid3<double>::index_type &i) {
        return double(adt::sum(i));
      },
      adt::steprange_t<3>(adt::index_t<3>{{s, s, s}}));
  static_assert(std::is_same<decltype(rs1), grid3<double>>::value, "");
  EXPECT_EQ(s * s * s, rs.size());
  EXPECT_EQ(27, rs.last());
}

TEST(fun_grid, dump) {
  std::ptrdiff_t s = 3;
  auto rs = iotaMapMulti<grid2<adt::dummy>>(
      [](const auto &i) { return int(adt::sum(i)); },
      adt::steprange_t<2>(adt::index_t<2>{{s, s}}));
  std::string str(dump(rs));
  EXPECT_EQ("grid{[[0,1,2,],[1,2,3,],[2,3,4,],],}", str);
}

TEST(fun_grid, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMapMulti<grid3<adt::dummy>>(
      [](const typename grid3<int>::index_type &x) { return int(adt::sum(x)); },
      adt::steprange_t<3>(adt::index_t<3>{{s, s, s}}));
  EXPECT_EQ(27, xs.last());

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);
  EXPECT_EQ(28, ys.last());

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_EQ(55, zs.last());
}

TEST(fun_grid, boundary) {
  std::ptrdiff_t s = 10;

  auto xs1 = iotaMapMulti<grid1<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<1>(adt::index_t<1>{{s}}));
  std::array<grid0<int>, 2> bxs1;
  for (std::ptrdiff_t i = 0; i < 2; ++i) {
    bxs1[i] = fun::boundary(xs1, i);
    EXPECT_EQ(1, fun::msize(bxs1[i]));
  }
  EXPECT_EQ(0, fun::mextract(bxs1[0]));
  EXPECT_EQ((s - 1) * (s - 1), fun::mextract(bxs1[1]));

  auto xs2 = iotaMapMulti<grid2<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<2>(adt::index_t<2>{{s, s}}));
  std::array<grid1<int>, 4> bxs2;
  for (std::ptrdiff_t i = 0; i < 4; ++i) {
    bxs2[i] = fun::boundary(xs2, i);
    EXPECT_EQ(s, fun::msize(bxs2[i]));
  }
  EXPECT_EQ(0 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[0], 0)));
  EXPECT_EQ(1 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[0], 1)));
  EXPECT_EQ(0 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[2], 0)));
  EXPECT_EQ(1 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[2], 1)));
  EXPECT_EQ(1 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[1], 0)));
  EXPECT_EQ(2 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[1], 1)));
  EXPECT_EQ(1 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[3], 0)));
  EXPECT_EQ(2 * (s - 1) * (s - 1), fun::mextract(fun::boundary(bxs2[3], 1)));
}

TEST(fun_grid, boundaryMap) {
  std::ptrdiff_t s = 10;

  auto xs1 = iotaMapMulti<grid1<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<1>(adt::index_t<1>{{s}}));
  std::array<grid0<int>, 2> bxs1;
  for (std::ptrdiff_t i = 0; i < 2; ++i) {
    bxs1[i] = fun::boundaryMap([](auto x, auto i) { return -x; }, xs1, i);
    EXPECT_EQ(1, fun::msize(bxs1[i]));
  }
  EXPECT_EQ(0, fun::mextract(bxs1[0]));
  EXPECT_EQ(-(s - 1) * (s - 1), fun::mextract(bxs1[1]));

  auto xs2 = iotaMapMulti<grid2<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<2>(adt::index_t<2>{{s, s}}));
  std::array<grid1<int>, 4> bxs2;
  for (std::ptrdiff_t i = 0; i < 4; ++i) {
    bxs2[i] = fun::boundaryMap([](auto x, auto i) { return -x; }, xs2, i);
    EXPECT_EQ(s, fun::msize(bxs2[i]));
  }
  EXPECT_EQ(0, fun::mextract(bxs2[0]));
  EXPECT_EQ(0, fun::mextract(bxs2[2]));
  EXPECT_EQ(-(s - 1) * (s - 1), fun::mextract(bxs2[1]));
  EXPECT_EQ(-(s - 1) * (s - 1), fun::mextract(bxs2[3]));
}

TEST(fun_grid, fmapStencil) {
  std::ptrdiff_t s = 10;

  auto xs0 = iotaMapMulti<grid0<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<0>(adt::index_t<0>{{}}));
  auto ys0 = fmapStencilMulti<0>([](auto x, auto bdirs) { return 0; },
                                 [](auto x, auto i) { return x; }, xs0, ~0);
  auto sum0 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys0);
  EXPECT_EQ(0, sum0);

  auto xs1 = iotaMapMulti<grid1<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<1>(adt::index_t<1>{{s}}));
  auto bms1 = iotaMapMulti<grid0<adt::dummy>>(
      [](const auto &x) { return int(-1 * -1); },
      adt::steprange_t<0>(adt::index_t<0>{{}}));
  auto bps1 =
      iotaMapMulti<grid0<adt::dummy>>([s](const auto &x) { return int(s * s); },
                                      adt::steprange_t<0>(adt::index_t<0>{{}}));
  auto ys1 = fmapStencilMulti<1>(
      // [](auto x, auto bdirs, auto bm0, auto bp0) { return bm0 - 2 * x + bp0;
      // },
      [](auto x, auto bdirs, int bm0, int bp0) { return bm0 - 2 * x + bp0; },
      [](auto x, auto i) { return x; }, xs1, ~0, bms1, bps1);
  auto sum1 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys1);
  EXPECT_EQ(20, sum1);

  auto xs2 = iotaMapMulti<grid2<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x * x)); },
      adt::steprange_t<2>(adt::index_t<2>{{s, s}}));
  auto bms2 = iotaMapMulti<grid1<adt::dummy>>(
      [](const auto &x) { return int(-1 * -1 + adt::sum(x * x)); },
      adt::steprange_t<1>(adt::index_t<1>{{s}}));
  auto bps2 = iotaMapMulti<grid1<adt::dummy>>(
      [s](const auto &x) { return int(s * s + adt::sum(x * x)); },
      adt::steprange_t<1>(adt::index_t<1>{{s}}));
  auto ys2 = fmapStencilMulti<2>(
      // [](auto x, auto bdirs, auto bm0, auto bm1, auto bp0, auto bp1) {
      //   return (bm0 - 2 * x + bp0) + (bm1 - 2 * x + bp1);
      // },
      [](auto x, auto bdirs, int bm0, int bm1, int bp0, int bp1) {
        return (bm0 - 2 * x + bp0) + (bm1 - 2 * x + bp1);
      },
      [](auto x, auto i) { return x; }, xs2, ~0, bms2, bms2, bps2, bps2);
  auto sum2 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, ys2);
  EXPECT_EQ(400, sum2);
}

TEST(fun_grid, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMapMulti<grid3<adt::dummy>>(
      [](const auto &x) { return int(adt::sum(x)); },
      adt::steprange_t<3>(adt::index_t<3>{{s, s, s}}));
  EXPECT_EQ(27, xs.last());
  auto r = foldMap([](auto x) { return x; },
                   [](auto x, auto y) { return x + y; }, 0, xs);
  EXPECT_EQ(13500, r);
}

TEST(fun_grid, monad) {
  auto xs = munit<grid3<adt::dummy>>(1);
  auto xss = munit<grid3<adt::dummy>>(xs);
  //   auto ys = mjoin(xss);
  //   auto zs = mbind([](auto x) { return munit<grid3>(x); }, ys);
  //   auto z = mextract(zs);
  //   EXPECT_EQ(1, z);

  // auto z1 = mfoldMap([](auto x) { return x; },
  //                    [](auto x, auto y) { return x + y; }, 0, zs);
  // EXPECT_EQ(1, z1.size());
  // EXPECT_EQ(z, mextract(z1));

  auto ns = mzero<grid3<adt::dummy>, int>();
  EXPECT_TRUE(ns.empty());
  //   auto ps = mplus(ns, xs, ys, zs);
  //   EXPECT_EQ(3, ps.size());
  //   auto ss = msome<grid3>(1, 2, 3);
  //   EXPECT_EQ(3, ss.size());
  EXPECT_TRUE(mempty(ns));
  //   EXPECT_FALSE(mempty(ps));
  //   EXPECT_FALSE(mempty(ss));
}
