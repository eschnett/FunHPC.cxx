#include <adt/grid2_decl.hpp>

#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <fun/vector.hpp>

#include <adt/grid2_impl.hpp>

#include <gtest/gtest.h>

#include <cmath>

TEST(adt_grid2, basic) {
  auto g0 = adt::grid2<std::vector<adt::dummy>, double, 0>();
  EXPECT_EQ(1, g0.size());
  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>();
  EXPECT_EQ(0, g1.size());
  auto g2 = adt::grid2<std::vector<adt::dummy>, double, 2>();
  EXPECT_EQ(0, g2.size());
  auto g3 = adt::grid2<std::vector<adt::dummy>, double, 3>();
  EXPECT_EQ(0, g3.size());
  auto g10 = adt::grid2<std::vector<adt::dummy>, double, 10>();
  EXPECT_EQ(0, g10.size());
}

TEST(adt_grid2, iotaMap_simple) {
  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::iotaMap(),
      [](auto i) { return double(i); }, adt::irange_t(10));
  EXPECT_EQ(9.0, g1.last());
}

TEST(adt_grid2, iotaMap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 0>(s));
  EXPECT_EQ(0 * double(s - 1), g0.last());
  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 1>(s));
  EXPECT_EQ(1 * double(s - 1), g1.last());
  auto g2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 2>(s));
  EXPECT_EQ(2 * double(s - 1), g2.last());
  auto g3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 3>(s));
  EXPECT_EQ(3 * double(s - 1), g3.last());
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 10>(s10));
  EXPECT_EQ(10 * double(s10 - 1), g10.last());
}

TEST(adt_grid2, fmap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 0>(s));
  auto x0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::fmap(),
      [](auto x) { return 2 * x + 1; }, g0);
  EXPECT_EQ(2 * (0 * double(s - 1)) + 1, x0.last());
  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 1>(s));
  auto x1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::fmap(),
      [](auto x) { return 2 * x + 1; }, g1);
  EXPECT_EQ(2 * (1 * double(s - 1)) + 1, x1.last());
  auto g2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 2>(s));
  auto x2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::fmap(),
      [](auto x) { return 2 * x + 1; }, g2);
  EXPECT_EQ(2 * (2 * double(s - 1)) + 1, x2.last());
  auto g3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 3>(s));
  auto x3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::fmap(),
      [](auto x) { return 2 * x + 1; }, g3);
  EXPECT_EQ(2 * (3 * double(s - 1)) + 1, x3.last());
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 10>(s10));
  auto x10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::fmap(),
      [](auto x) { return 2 * x + 1; }, g10);
  EXPECT_EQ(2 * (10 * double(s10 - 1)) + 1, x10.last());
}

TEST(adt_grid2, fmap2) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 0>(s));
  auto x0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::fmap(),
      [](auto x) { return 2 * x + 1; }, g0);
  auto y0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::fmap2(),
      [](auto x, auto y) { return x + y; }, g0, x0);
  EXPECT_EQ(3 * (0 * double(s - 1)) + 1, y0.last());

  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 1>(s));
  auto x1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::fmap(),
      [](auto x) { return 2 * x + 1; }, g1);
  auto y1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::fmap2(),
      [](auto x, auto y) { return x + y; }, g1, x1);
  EXPECT_EQ(3 * (1 * double(s - 1)) + 1, y1.last());

  auto g2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 2>(s));
  auto x2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::fmap(),
      [](auto x) { return 2 * x + 1; }, g2);
  auto y2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::fmap2(),
      [](auto x, auto y) { return x + y; }, g2, x2);
  EXPECT_EQ(3 * (2 * double(s - 1)) + 1, y2.last());

  auto g3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 3>(s));
  auto x3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::fmap(),
      [](auto x) { return 2 * x + 1; }, g3);
  auto y3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::fmap2(),
      [](auto x, auto y) { return x + y; }, g3, x3);
  EXPECT_EQ(3 * (3 * double(s - 1)) + 1, y3.last());

  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::array_set<std::ptrdiff_t, 10>(s10));
  auto x10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::fmap(),
      [](auto x) { return 2 * x + 1; }, g10);
  auto y10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::fmap2(),
      [](auto x, auto y) { return x + y; }, g10, x10);
  EXPECT_EQ(3 * (10 * double(s10 - 1)) + 1, y10.last());
}

TEST(adt_grid2, fmapStencilMulti) {
  std::ptrdiff_t s = 10;

  typedef adt::grid2<std::vector<adt::dummy>, double, 0> grid2_0;
  auto g0 =
      grid2_0(typename grid2_0::iotaMapMulti(),
              [](auto i) { return double(adt::sum(i)); }, adt::index_t<0>{});
  auto x0 = grid2_0(typename grid2_0::fmapStencilMulti(),
                    [](auto x, auto bs) { return 0.0; },
                    [](auto x, auto f, auto d) {
                      cxx_assert(false);
                      return x;
                    },
                    g0, std::array<std::array<grid2_0, 0>, 2>{});
  EXPECT_EQ(x0.size(), g0.size());
  EXPECT_EQ(x0.active(), g0.active());
  auto maxabs0 =
      x0.foldMap([](auto x) { return std::fabs(x); },
                 [](auto x, auto y) { return std::fmax(x, y); }, 0.0);
  EXPECT_EQ(0.0, maxabs0);

  typedef adt::grid2<std::vector<adt::dummy>, double, 1> grid2_1;
  auto g1 =
      grid2_1(typename grid2_1::iotaMapMulti(),
              [](auto i) { return double(adt::sum(i)); }, adt::index_t<1>{s});
  auto bm1 = grid2_1(typename grid2_1::iotaMapMulti(),
                     [](auto i) { return double(adt::sum(i)); },
                     adt::range_t<1>(adt::index_t<1>{-1}, adt::index_t<1>{0}));
  auto bp1 =
      grid2_1(typename grid2_1::iotaMapMulti(),
              [](auto i) { return double(adt::sum(i)); },
              adt::range_t<1>(adt::index_t<1>{s}, adt::index_t<1>{s + 1}));
  auto x1 =
      grid2_1(typename grid2_1::fmapStencilMulti(),
              [](auto x, auto bs) { return bs[0][0] - 2.0 * x + bs[1][0]; },
              [](auto x, auto f, auto d) { return x; }, g1,
              std::array<std::array<grid2_1, 1>, 2>{bm1, bp1});
  EXPECT_EQ(x1.size(), g1.size());
  EXPECT_EQ(x1.active(), g1.active());
  auto maxabs1 =
      x1.foldMap([](auto x) { return std::fabs(x); },
                 [](auto x, auto y) { return std::fmax(x, y); }, 0.0);
  EXPECT_EQ(0.0, maxabs1);

  typedef adt::grid2<std::vector<adt::dummy>, double, 2> grid2_2;
  auto g2 = grid2_2(typename grid2_2::iotaMapMulti(),
                    [](auto i) { return double(adt::sum(i)); },
                    adt::index_t<2>{s, s});
  auto bm2_0 =
      grid2_2(typename grid2_2::iotaMapMulti(),
              [](auto i) { return double(adt::sum(i)); },
              adt::range_t<2>(adt::index_t<2>{-1, 0}, adt::index_t<2>{0, s}));
  auto bm2_1 =
      grid2_2(typename grid2_2::iotaMapMulti(),
              [](auto i) { return double(adt::sum(i)); },
              adt::range_t<2>(adt::index_t<2>{0, -1}, adt::index_t<2>{s, 0}));
  auto bp2_0 = grid2_2(
      typename grid2_2::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::range_t<2>(adt::index_t<2>{s, 0}, adt::index_t<2>{s + 1, s}));
  auto bp2_1 = grid2_2(
      typename grid2_2::iotaMapMulti(),
      [](auto i) { return double(adt::sum(i)); },
      adt::range_t<2>(adt::index_t<2>{0, s}, adt::index_t<2>{s, s + 1}));
  auto x2 = grid2_2(
      typename grid2_2::fmapStencilMulti(),
      [](auto x, auto bs) {
        return (bs[0][0] - 2.0 * x + bs[1][0]) +
               (bs[0][1] - 2.0 * x + bs[1][1]);
      },
      [](auto x, auto f, auto d) { return x; }, g2,
      std::array<std::array<grid2_2, 2>, 2>{bm2_0, bm2_1, bp2_0, bp2_1});
  EXPECT_EQ(x2.size(), g2.size());
  EXPECT_EQ(x2.active(), g2.active());
  auto maxabs2 =
      x2.foldMap([](auto x) { return std::fabs(x); },
                 [](auto x, auto y) { return std::fmax(x, y); }, 0.0);
  EXPECT_EQ(0.0, maxabs2);
}

TEST(adt_grid2, foldMap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid2<std::vector<adt::dummy>, double, 0>(
      typename adt::grid2<std::vector<adt::dummy>, double, 0>::iotaMapMulti(),
      [](auto x) { return double(adt::sum(x)); },
      adt::array_set<std::ptrdiff_t, 0>(s));
  auto r0 = g0.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r0), double>::value));
  EXPECT_EQ(0.0, r0);
  auto g1 = adt::grid2<std::vector<adt::dummy>, double, 1>(
      typename adt::grid2<std::vector<adt::dummy>, double, 1>::iotaMapMulti(),
      [](auto x) { return double(adt::sum(x)); },
      adt::array_set<std::ptrdiff_t, 1>(s));
  auto r1 = g1.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r1), double>::value));
  EXPECT_EQ(45.0, r1);
  auto g2 = adt::grid2<std::vector<adt::dummy>, double, 2>(
      typename adt::grid2<std::vector<adt::dummy>, double, 2>::iotaMapMulti(),
      [](auto x) { return double(adt::sum(x)); },
      adt::array_set<std::ptrdiff_t, 2>(s));
  auto r2 = g2.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r2), double>::value));
  EXPECT_EQ(900.0, r2);
  auto g3 = adt::grid2<std::vector<adt::dummy>, double, 3>(
      typename adt::grid2<std::vector<adt::dummy>, double, 3>::iotaMapMulti(),
      [](auto x) { return double(adt::sum(x)); },
      adt::array_set<std::ptrdiff_t, 3>(s));
  auto r3 = g3.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r3), double>::value));
  EXPECT_EQ(13500.0, r3);
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid2<std::vector<adt::dummy>, double, 10>(
      typename adt::grid2<std::vector<adt::dummy>, double, 10>::iotaMapMulti(),
      [](auto x) { return double(adt::sum(x)); },
      adt::array_set<std::ptrdiff_t, 10>(s10));
  auto r10 = g10.foldMap([](auto x) { return x; },
                         [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r10), double>::value));
  EXPECT_EQ(5120.0, r10);
}
