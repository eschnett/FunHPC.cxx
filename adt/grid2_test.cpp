#include <adt/grid2_decl.hpp>

#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <fun/vector.hpp>

#include <adt/grid2_impl.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <tuple>

namespace {
template <typename T, std::size_t D>
using vgrid = adt::grid2<std::vector<adt::dummy>, T, D>;
}

TEST(adt_grid2, basic) {
  auto g0 = vgrid<double, 0>();
  EXPECT_EQ(1, g0.size());
  auto g1 = vgrid<double, 1>();
  EXPECT_EQ(0, g1.size());
  auto g2 = vgrid<double, 2>();
  EXPECT_EQ(0, g2.size());
  auto g3 = vgrid<double, 3>();
  EXPECT_EQ(0, g3.size());
  auto g10 = vgrid<double, 10>();
  EXPECT_EQ(0, g10.size());
}

TEST(adt_grid2, iotaMap_simple) {
  auto g1 =
      vgrid<double, 1>(typename vgrid<double, 1>::iotaMap(),
                       [](auto i) { return double(i); }, adt::irange_t(10));
  EXPECT_EQ(9.0, g1.last());
}

namespace {
template <std::size_t D> void test_iotaMap() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  EXPECT_EQ(D * double(s - 1), g.last());
}
}

TEST(adt_grid2, iotaMap) {
  test_iotaMap<0>();
  test_iotaMap<1>();
  test_iotaMap<2>();
  test_iotaMap<3>();
  test_iotaMap<10>();
}

namespace {
template <std::size_t D> void test_fmap() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  auto x = vgrid<double, D>(typename vgrid<double, D>::fmap(),
                            [](auto x) { return 2 * x + 1; }, g);
  EXPECT_EQ(2 * (D * double(s - 1)) + 1, x.last());
}
}

TEST(adt_grid2, fmap) {
  test_fmap<0>();
  test_fmap<1>();
  test_fmap<2>();
  test_fmap<3>();
  test_fmap<10>();
}

namespace {
template <std::size_t D> void test_fmap2() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  auto x = vgrid<double, D>(typename vgrid<double, D>::fmap(),
                            [](auto x) { return 2 * x + 1; }, g);
  auto y = vgrid<double, D>(typename vgrid<double, D>::fmap2(),
                            [](auto x, auto y) { return x + y; }, g, x);
  EXPECT_EQ(3 * (D * double(s - 1)) + 1, y.last());
}
}

TEST(adt_grid2, fmap2) {
  test_fmap2<0>();
  test_fmap2<1>();
  test_fmap2<2>();
  test_fmap2<3>();
  test_fmap2<10>();
}

namespace {
template <std::size_t D> void test_fmap3() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  auto x = vgrid<double, D>(typename vgrid<double, D>::fmap(),
                            [](auto x) { return 2 * x + 1; }, g);
  auto y = vgrid<double, D>(typename vgrid<double, D>::fmap2(),
                            [](auto x, auto y) { return x + y; }, g, x);
  auto z = vgrid<double, D>(typename vgrid<double, D>::fmap3(),
                            [](auto x, auto y, auto z) { return x + y + z; }, g,
                            x, y);
  EXPECT_EQ(6 * (D * double(s - 1)) + 2, z.last());
}
}

TEST(adt_grid2, fmap3) {
  test_fmap3<0>();
  test_fmap3<1>();
  test_fmap3<2>();
  test_fmap3<3>();
  test_fmap3<10>();
}

namespace {
template <std::size_t D> void test_boundary() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  for (int f = 0; f < 2; ++f) {
    for (int d = 0; d < int(D); ++d) {
      auto bs =
          vgrid<double, D>(typename vgrid<double, D>::boundary(), g, f, d);
      EXPECT_EQ(std::pow(s, D - 1), bs.size());
      if (f == 0) {
        EXPECT_EQ(0, bs.head());
        EXPECT_EQ((D - 1) * (s - 1), bs.last());
      } else {
        EXPECT_EQ(s - 1, bs.head());
        EXPECT_EQ(D * (s - 1), bs.last());
      }
    }
  }
}
}

TEST(adt_grid2, boundary) {
  test_boundary<1>();
  test_boundary<2>();
  test_boundary<3>();
  test_boundary<10>();
}

namespace {
template <std::size_t D> void test_fmapStencil() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto i) { return double(adt::sum(i)); },
                            adt::set<adt::index_t<D>>(s));
  std::array<std::array<vgrid<double, D>, D>, 2> bss;
  for (int f = 0; f < 2; ++f) {
    for (int d = 0; d < int(D); ++d) {
      auto bmin = adt::set<adt::index_t<D>>(0);
      auto bmax = adt::set<adt::index_t<D>>(s);
      if (f == 0) {
        bmin[d] = -1;
        bmax[d] = 0;
      } else {
        bmin[d] = s;
        bmax[d] = s + 1;
      }
      bss[f][d] = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                                   [](auto i) { return double(adt::sum(i)); },
                                   adt::range_t<D>(bmin, bmax));
    }
  }
  auto x = vgrid<double, D>(typename vgrid<double, D>::fmapStencil(),
                            [](auto x, auto bs) {
                              auto r = x;
                              r = 0;
                              for (int d = 0; d < int(D); ++d)
                                r += bs[0][d] - 2 * x + bs[1][d];
                              return r;
                            },
                            [](auto x, auto f, auto d) { return x; }, g, bss);
  EXPECT_EQ(x.size(), g.size());
  EXPECT_EQ(x.active(), g.active());
  double maxabs = 0.0;
  vgrid<std::tuple<>, D>(typename vgrid<std::tuple<>, D>::fmap(),
                         [&](auto x) {
                           maxabs = std::fmax(maxabs, std::fabs(x));
                           return std::tuple<>();
                         },
                         x);
  EXPECT_EQ(0.0, maxabs);
}
}

TEST(adt_grid2, fmapStencil) {
  test_fmapStencil<0>();
  test_fmapStencil<1>();
  test_fmapStencil<2>();
  test_fmapStencil<3>();
  test_fmapStencil<10>();
}

namespace {
template <std::size_t D> void test_foldMap() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = vgrid<double, D>(typename vgrid<double, D>::iotaMap(),
                            [](auto x) { return double(adt::sum(x)); },
                            adt::set<adt::index_t<D>>(s));
  auto r = g.foldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r), double>::value));
  EXPECT_EQ(D == 0 ? 0 : D * std::pow(s, D - 1) * s * (s - 1) / 2, r);
}
}

TEST(adt_grid2, foldMap) {
  test_foldMap<0>();
  test_foldMap<1>();
  test_foldMap<2>();
  test_foldMap<3>();
  test_foldMap<10>();
}
