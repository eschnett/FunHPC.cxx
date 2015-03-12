#include <fun/vector.hpp>

#include <adt/grid.hpp>

#include <gtest/gtest.h>

namespace {
template <typename T> using vector1 = std::vector<T>;
}

TEST(adt_grid, basic) {
  auto g0 = adt::grid<vector1, double, 0>();
  EXPECT_EQ(1, g0.size());
  auto g1 = adt::grid<vector1, double, 1>();
  EXPECT_EQ(0, g1.size());
  auto g2 = adt::grid<vector1, double, 2>();
  EXPECT_EQ(0, g2.size());
  auto g3 = adt::grid<vector1, double, 3>();
  EXPECT_EQ(0, g3.size());
  auto g10 = adt::grid<vector1, double, 10>();
  EXPECT_EQ(0, g10.size());
}

TEST(adt_grid, iotaMap_simple) {
  auto g0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::iotaMap(),
      [](auto i) { return double(i); }, 1);
  EXPECT_EQ(0.0, g0.last());
  auto g1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::iotaMap(),
      [](auto i) { return double(i); }, 10);
  EXPECT_EQ(9.0, g1.last());
}

TEST(adt_grid, iotaMap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 0>(s));
  EXPECT_EQ(0 * double(s - 1), g0.last());
  auto g1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 1>(s));
  EXPECT_EQ(1 * double(s - 1), g1.last());
  auto g2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 2>(s));
  EXPECT_EQ(2 * double(s - 1), g2.last());
  auto g3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 3>(s));
  EXPECT_EQ(3 * double(s - 1), g3.last());
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 10>(s10));
  EXPECT_EQ(10 * double(s10 - 1), g10.last());
}

TEST(adt_grid, fmap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 0>(s));
  auto x0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::fmap(),
      [](auto x) { return 2 * x + 1; }, g0);
  EXPECT_EQ(2 * (0 * double(s - 1)) + 1, x0.last());
  auto g1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 1>(s));
  auto x1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::fmap(),
      [](auto x) { return 2 * x + 1; }, g1);
  EXPECT_EQ(2 * (1 * double(s - 1)) + 1, x1.last());
  auto g2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 2>(s));
  auto x2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::fmap(),
      [](auto x) { return 2 * x + 1; }, g2);
  EXPECT_EQ(2 * (2 * double(s - 1)) + 1, x2.last());
  auto g3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 3>(s));
  auto x3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::fmap(),
      [](auto x) { return 2 * x + 1; }, g3);
  EXPECT_EQ(2 * (3 * double(s - 1)) + 1, x3.last());
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 10>(s10));
  auto x10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::fmap(),
      [](auto x) { return 2 * x + 1; }, g10);
  EXPECT_EQ(2 * (10 * double(s10 - 1)) + 1, x10.last());
}

TEST(adt_grid, fmap2) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 0>(s));
  auto x0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::fmap(),
      [](auto x) { return 2 * x + 1; }, g0);
  auto y0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::fmap2(),
      [](auto x, auto y) { return x + y; }, g0, x0);
  EXPECT_EQ(3 * (0 * double(s - 1)) + 1, y0.last());
  auto g1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 1>(s));
  auto x1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::fmap(),
      [](auto x) { return 2 * x + 1; }, g1);
  auto y1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::fmap2(),
      [](auto x, auto y) { return x + y; }, g1, x1);
  EXPECT_EQ(3 * (1 * double(s - 1)) + 1, y1.last());
  auto g2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 2>(s));
  auto x2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::fmap(),
      [](auto x) { return 2 * x + 1; }, g2);
  auto y2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::fmap2(),
      [](auto x, auto y) { return x + y; }, g2, x2);
  EXPECT_EQ(3 * (2 * double(s - 1)) + 1, y2.last());
  auto g3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 3>(s));
  auto x3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::fmap(),
      [](auto x) { return 2 * x + 1; }, g3);
  auto y3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::fmap2(),
      [](auto x, auto y) { return x + y; }, g3, x3);
  EXPECT_EQ(3 * (3 * double(s - 1)) + 1, y3.last());
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::iotaMap(), [](auto i) {
        return double(adt::sum(i));
      }, adt::array_fill<std::ptrdiff_t, 10>(s10));
  auto x10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::fmap(),
      [](auto x) { return 2 * x + 1; }, g10);
  auto y10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::fmap2(),
      [](auto x, auto y) { return x + y; }, g10, x10);
  EXPECT_EQ(3 * (10 * double(s10 - 1)) + 1, y10.last());
}

TEST(adt_grid, foldMap) {
  std::ptrdiff_t s = 10;
  auto g0 = adt::grid<vector1, double, 0>(
      typename adt::grid<vector1, double, 0>::iotaMap(), [](auto x) {
        return double(adt::sum(x));
      }, adt::array_fill<std::ptrdiff_t, 0>(s));
  auto r0 = g0.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r0), double>::value));
  EXPECT_EQ(0.0, r0);
  auto g1 = adt::grid<vector1, double, 1>(
      typename adt::grid<vector1, double, 1>::iotaMap(), [](auto x) {
        return double(adt::sum(x));
      }, adt::array_fill<std::ptrdiff_t, 1>(s));
  auto r1 = g1.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r1), double>::value));
  EXPECT_EQ(45.0, r1);
  auto g2 = adt::grid<vector1, double, 2>(
      typename adt::grid<vector1, double, 2>::iotaMap(), [](auto x) {
        return double(adt::sum(x));
      }, adt::array_fill<std::ptrdiff_t, 2>(s));
  auto r2 = g2.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r2), double>::value));
  EXPECT_EQ(900.0, r2);
  auto g3 = adt::grid<vector1, double, 3>(
      typename adt::grid<vector1, double, 3>::iotaMap(), [](auto x) {
        return double(adt::sum(x));
      }, adt::array_fill<std::ptrdiff_t, 3>(s));
  auto r3 = g3.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r3), double>::value));
  EXPECT_EQ(13500.0, r3);
  std::ptrdiff_t s10 = 2;
  auto g10 = adt::grid<vector1, double, 10>(
      typename adt::grid<vector1, double, 10>::iotaMap(), [](auto x) {
        return double(adt::sum(x));
      }, adt::array_fill<std::ptrdiff_t, 10>(s10));
  auto r10 = g10.foldMap([](auto x) { return x; },
                         [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_TRUE((std::is_same<decltype(r10), double>::value));
  EXPECT_EQ(5120.0, r10);
}
