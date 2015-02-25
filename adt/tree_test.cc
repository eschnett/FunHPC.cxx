#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <adt/nested.hpp>
#include <fun/nested.hpp>

#include <adt/tree.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {
template <typename T> using vector1 = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, vector1, T>;
template <typename T> using tree1 = adt::tree<shared_vector, T>;
}

TEST(adt_tree, basic) {
  auto xs = tree1<double>();
  EXPECT_TRUE(xs.empty());
  auto ys = tree1<double>(1.0);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(1.0, ys.extract());

  auto zs1(xs);
  auto zs2(std::move(ys));
  zs1 = xs;
  zs1 = zs1;
  zs1 = std::move(zs2);
  using std::swap;
  swap(xs, zs1);

  auto t0 = tree1<double>();
  auto t1 = tree1<double>(1.0);
  auto t2 = tree1<double>(2.0);
  auto t3 = tree1<double>(t0, t1, t2);
  auto t4 = tree1<double>(t2, t3);
  auto t5 = tree1<double>(t0, t1, t4);
}

TEST(adt_tree, iota) {
  auto xs = tree1<double>(typename tree1<double>::iotaMap(),
                          [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.extract());
  auto ys = tree1<double>(typename tree1<double>::iotaMap(),
                          [](auto i) { return double(i); }, 0, 100, 1);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(0.0, ys.extract());
}

TEST(adt_tree, fmap) {
  auto xs = tree1<double>(typename tree1<double>::iotaMap(),
                          [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.extract());
  auto ys = tree1<double>(typename tree1<double>::fmap(),
                          [](auto x, auto y) { return x + y; }, xs, 1.0);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(1.0, ys.extract());
  auto zs = tree1<double>(typename tree1<double>::fmap2(),
                          [](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_FALSE(zs.empty());
  EXPECT_EQ(1.0, zs.extract());
}

TEST(adt_tree, foldMap) {
  auto xs = tree1<double>(typename tree1<double>::iotaMap(),
                          [](auto i) { return double(i); }, 0, 10, 1);
  EXPECT_FALSE(xs.empty());
  EXPECT_EQ(0.0, xs.extract());
  auto ys = tree1<double>(typename tree1<double>::iotaMap(),
                          [](auto i) { return double(i); }, 0, 100, 1);
  EXPECT_FALSE(ys.empty());
  EXPECT_EQ(0.0, ys.extract());

  auto rx = xs.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_EQ(45.0, rx);
  auto ry = ys.foldMap([](auto x) { return x; },
                       [](auto x, auto y) { return x + y; }, 0.0);
  EXPECT_EQ(4950.0, ry);

  EXPECT_EQ(0, tree1<double>().size());
  EXPECT_EQ(1, tree1<double>(3.0).size());
  EXPECT_EQ(10, xs.size());
  EXPECT_EQ(100, ys.size());
}

TEST(adt_tree, join) {
  auto xss = tree1<tree1<double>>(tree1<double>(1.0));
  EXPECT_EQ(1, xss.size());
  auto xs = tree1<double>(typename tree1<double>::join(), xss);
  EXPECT_EQ(1, xs.size());
  EXPECT_EQ(1, xs.extract());

  auto yss = tree1<tree1<double>>(
      typename tree1<tree1<double>>::iotaMap(),
      [](auto i) {
        return tree1<double>(typename tree1<double>::iotaMap(),
                             [](auto i) { return double(i); }, 0, i, 1);
      },
      0, 100, 1);
  EXPECT_EQ(100, yss.size());
  auto ys = tree1<double>(typename tree1<double>::join(), yss);
  EXPECT_EQ(4950, ys.size());
}
