#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <adt/nested.hpp>
#include <fun/nested.hpp>

#include <adt/tree.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

namespace {
template <typename T> using std_vector = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, std_vector, T>;
template <typename T> using vector_tree = adt::tree<std_vector, T>;
template <typename T> using shared_tree = adt::tree<shared_vector, T>;
}

TEST(adt_tree, basic) {
  auto xs = vector_tree<double>();
  EXPECT_TRUE(xs.empty());
  auto ys = vector_tree<double>(1.0);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(1.0, ys.head());

  auto zs1(xs);
  auto zs2(std::move(ys));
  zs1 = xs;
  zs1 = zs1;
  zs1 = std::move(zs2);
  using std::swap;
  swap(xs, zs1);

  auto t0 = vector_tree<double>();
  EXPECT_EQ(0, t0.size());
  auto t1 = vector_tree<double>(1.0);
  EXPECT_EQ(1, t1.size());
  auto t2 = vector_tree<double>(2.0);
  EXPECT_EQ(1, t2.size());
  auto t3 = vector_tree<double>(t0, t1, t2);
  EXPECT_EQ(2, t3.size());
  auto t4 = vector_tree<double>(t2, t3);
  EXPECT_EQ(3, t4.size());
  auto t5 = vector_tree<double>(t0, t1, t4);
  EXPECT_EQ(4, t5.size());
}

TEST(adt_tree, basic_nested) {
  auto xs = shared_tree<double>();
  EXPECT_TRUE(xs.empty());
  auto ys = shared_tree<double>(1.0);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(1.0, ys.head());

  auto zs1(xs);
  auto zs2(std::move(ys));
  zs1 = xs;
  zs1 = zs1;
  zs1 = std::move(zs2);
  using std::swap;
  swap(xs, zs1);

  auto t0 = shared_tree<double>();
  EXPECT_EQ(0, t0.size());
  auto t1 = shared_tree<double>(1.0);
  EXPECT_EQ(1, t1.size());
  auto t2 = shared_tree<double>(2.0);
  EXPECT_EQ(1, t2.size());
  auto t3 = shared_tree<double>(t0, t1, t2);
  EXPECT_EQ(2, t3.size());
  auto t4 = shared_tree<double>(t2, t3);
  EXPECT_EQ(3, t4.size());
  auto t5 = shared_tree<double>(t0, t1, t4);
  EXPECT_EQ(4, t5.size());
}

TEST(adt_tree, iota) {
  auto xs = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.head());
  auto ys = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 100, 1);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(0.0, ys.head());
}

TEST(adt_tree, fmap) {
  auto xs = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.head());
  auto ys = shared_tree<double>(typename shared_tree<double>::fmap(),
                                [](auto x, auto y) { return x + y; }, xs, 1.0);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(1.0, ys.head());
  auto zs = shared_tree<double>(typename shared_tree<double>::fmap2(),
                                [](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_FALSE(zs.empty());
  EXPECT_EQ(1.0, zs.head());
}

TEST(adt_tree, fmapStencil) {
  auto xs = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 1000, 1);
  auto ys = shared_tree<double>(
      typename shared_tree<double>::fmapStencil(),
      [](auto x, auto bdirs, auto bm, auto bp) { return bm - 2.0 * x + bp; },
      [](auto x, auto i) { return x; }, xs, 0b11, -1.0, 1000.0);
  EXPECT_EQ(xs.size(), ys.size());
  auto maxabs = ys.foldMap([](auto x) { return std::fabs(x); },
                           [](auto x, auto y) { return std::fmax(x, y); }, 0.0);
  EXPECT_EQ(0.0, maxabs);
}

TEST(adt_tree, foldMap) {
  auto xs = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.head());
  auto ys = shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                [](auto i) { return double(i); }, 0, 100, 1);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(0.0, ys.head());

  auto rx = xs.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_EQ(45.0, rx);
  auto ry = ys.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_EQ(4950.0, ry);

  EXPECT_EQ(0, shared_tree<double>().size());
  EXPECT_EQ(1, shared_tree<double>(3.0).size());
  EXPECT_EQ(10, xs.size());
  EXPECT_EQ(100, ys.size());
}

TEST(adt_tree, join) {
  auto xss = shared_tree<shared_tree<double>>(shared_tree<double>(1.0));
  EXPECT_EQ(1, xss.size());
  auto xs = shared_tree<double>(typename shared_tree<double>::join(), xss);
  EXPECT_EQ(1, xs.size());
  EXPECT_EQ(1, xs.head());

  auto yss = shared_tree<shared_tree<double>>(
      typename shared_tree<shared_tree<double>>::iotaMap(), [](auto i) {
        return shared_tree<double>(typename shared_tree<double>::iotaMap(),
                                   [](auto i) { return double(i); }, 0, i, 1);
      }, 0, 100, 1);
  EXPECT_EQ(100, yss.size());
  auto ys = shared_tree<double>(typename shared_tree<double>::join(), yss);
  EXPECT_EQ(4950, ys.size());
}
