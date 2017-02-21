#include <fun/grid2_decl.hpp>

#include <adt/arith.hpp>
#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <adt/maxarray.hpp>
#include <fun/dummy.hpp>
#include <fun/maxarray.hpp>
#include <fun/shared_future.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>
#include <qthread/future.hpp>

#include <fun/nested_decl.hpp>

#include <fun/nested_impl.hpp>

#include <fun/grid2_impl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <vector>

using namespace fun;

namespace {
template <typename T, std::size_t D>
using vgrid = adt::grid2<std::vector<adt::dummy>, T, D>;

template <typename T, std::size_t D>
using agrid = adt::grid2<adt::maxarray<adt::dummy, 10>, T, D>;

template <typename T>
using shared_vector =
    adt::nested<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T, std::size_t D>
using svgrid = adt::grid2<shared_vector<adt::dummy>, T, D>;

template <typename T>
using future_vector =
    adt::nested<qthread::shared_future<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T, std::size_t D>
using fvgrid = adt::grid2<future_vector<adt::dummy>, T, D>;
}

namespace {
std::ptrdiff_t get_size(std::size_t D, std::size_t max_size,
                        std::ptrdiff_t size) {
  cxx_assert(D >= 0);
  cxx_assert(size > 0);
  std::ptrdiff_t s = size;
  if (D > 0)
    s = std::lrint(std::pow(1000.0, 1.0 / D));
  cxx_assert(size > 0);
  s = std::max(std::ptrdiff_t(s), s);
  cxx_assert(size > 0);
  if (std::ptrdiff_t(max_size) >= 0)
    s = std::min(std::ptrdiff_t(max_size), s);
  cxx_assert(size > 0);
  return s;
}
}

namespace {
template <template <typename, std::size_t> typename grid, std::size_t D>
void test_iotaMap() {
  auto max_size = fun::fun_traits<grid<adt::dummy, D>>::max_size();
  std::ptrdiff_t max_dim_size =
      max_size >= 0 ? llrint(floor(pow(double(max_size), 1.0 / D))) : max_size;
  std::ptrdiff_t s = get_size(D, max_dim_size, 1000);
  auto xs = iotaMap<grid<adt::dummy, D>>(
      [](const adt::index_t<D> &i) { return double(adt::sum(i)); },
      adt::range_t<D>(adt::set<adt::index_t<D>>(s)));
  static_assert(std::is_same<decltype(xs), grid<double, D>>::value, "");
  EXPECT_EQ(std::pow(s, D), xs.size());
  EXPECT_EQ(D * (s - 1), xs.last());
}
}

TEST(fun_grid2, iotaMap) {
  {
    std::ptrdiff_t s = 10;
    auto xs = iotaMap<vgrid<adt::dummy, 1>>(
        [](std::ptrdiff_t i) { return double(i); }, adt::irange_t(s));
    static_assert(std::is_same<decltype(xs), vgrid<double, 1>>::value, "");
    EXPECT_EQ(s, xs.size());
    EXPECT_EQ(s - 1, xs.last());
  }

  test_iotaMap<vgrid, 0>();
  test_iotaMap<vgrid, 1>();
  test_iotaMap<vgrid, 2>();
  test_iotaMap<vgrid, 3>();
  test_iotaMap<vgrid, 10>();
  test_iotaMap<agrid, 0>();
  test_iotaMap<agrid, 1>();
  test_iotaMap<agrid, 2>();
  test_iotaMap<agrid, 3>();
  test_iotaMap<agrid, 10>();
  test_iotaMap<svgrid, 0>();
  test_iotaMap<svgrid, 1>();
  test_iotaMap<svgrid, 2>();
  test_iotaMap<svgrid, 3>();
  test_iotaMap<svgrid, 10>();
  // test_iotaMap<fvgrid, 0>();
  // test_iotaMap<fvgrid, 1>();
  // test_iotaMap<fvgrid, 2>();
  // test_iotaMap<fvgrid, 3>();
  // test_iotaMap<fvgrid, 10>();
}

namespace {
template <std::size_t D> void test_fmap() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto xs = iotaMap<vgrid<adt::dummy, D>>(
      [](const adt::index_t<D> &i) { return int(adt::sum(i)); },
      adt::range_t<D>(adt::set<adt::index_t<D>>(s)));
  auto rs = fmap([](int x) { return double(2 * x + 1); }, xs);
  static_assert(std::is_same<decltype(rs), vgrid<double, D>>::value, "");
  EXPECT_EQ(std::pow(s, D), rs.size());
  EXPECT_EQ(D * (2 * (s - 1)) + 1, rs.last());
  auto qs = fmap2([](int x, double r) { return r - (2 * x + 1); }, xs, rs);
  EXPECT_EQ(0, qs.head());
  EXPECT_EQ(0, qs.last());
  auto ps = fmap3([](int x, int y, int z) { return x + y + z; }, xs, xs, xs);
  EXPECT_EQ(0, ps.head());
  EXPECT_EQ(D * (3 * (s - 1)), ps.last());
}
}

TEST(fun_grid2, fmap) {
  test_fmap<0>();
  test_fmap<1>();
  test_fmap<2>();
  test_fmap<3>();
  test_fmap<10>();
}

namespace {
template <std::size_t D> void test_boundary() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = iotaMap<vgrid<adt::dummy, D>>(
      [](auto i) { return double(adt::sum(i)); },
      adt::range_t<D>(adt::set<adt::index_t<D>>(s)));
  for (int f = 0; f < 2; ++f) {
    for (int d = 0; d < int(D); ++d) {
      auto bs = boundary(g, f, d);
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

TEST(fun_grid2, boundary) {
  test_boundary<1>();
  test_boundary<2>();
  test_boundary<3>();
  test_boundary<10>();
}

namespace {
template <std::size_t D> void test_fmapStencil() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto g = iotaMap<vgrid<adt::dummy, D>>(
      [](auto i) { return double(adt::sum(i)); },
      adt::range_t<D>(adt::set<adt::index_t<D>>(s)));
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
      bss[f][d] = iotaMap<vgrid<adt::dummy, D>>(
          [](auto i) { return double(adt::sum(i)); },
          adt::range_t<D>(bmin, bmax));
    }
  }
  auto x = fmapStencil(
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
  fmap(
      [&](auto x) {
        maxabs = std::fmax(maxabs, std::fabs(x));
        return std::tuple<>();
      },
      x);
  EXPECT_EQ(0.0, maxabs);
}
}

TEST(fun_grid2, fmapStencil) {
  test_fmapStencil<0>();
  test_fmapStencil<1>();
  test_fmapStencil<2>();
  test_fmapStencil<3>();
  test_fmapStencil<10>();
}

namespace {
template <std::size_t D> void test_foldMap() {
  std::ptrdiff_t s = std::lrint(std::fmax(2.0, std::pow(1000.0, 1.0 / D)));
  auto xs = iotaMap<vgrid<adt::dummy, D>>(
      [](const adt::index_t<D> &i) { return int(adt::sum(i)); },
      adt::range_t<D>(adt::set<adt::index_t<D>>(s)));
  auto r = foldMap([](int x) { return double(x); },
                   [](double x, double y) { return x + y; }, 0.0, xs);
  static_assert(std::is_same<decltype(r), double>::value, "");
  EXPECT_EQ(D * std::pow(s, D - 1) * s * (s - 1) / 2, r);
}
}

TEST(fun_grid2, foldMap) {
  test_foldMap<0>();
  test_foldMap<1>();
  test_foldMap<2>();
  test_foldMap<3>();
  test_foldMap<10>();
}
