#include <fun/idtype.hpp>
#include <fun/shared_future.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>
#include <fun/nested.hpp>
#include <fun/tree.hpp>

#include <gtest/gtest.h>

using namespace fun;

namespace {
template <typename T>
using shared_vector =
    adt::nested<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T>
using future_vector =
    adt::nested<qthread::shared_future<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T>
using shared_tree = adt::tree<shared_vector<adt::dummy>, T>;
template <typename T>
using future_tree = adt::tree<future_vector<adt::dummy>, T>;
}

#warning "TODO: Test trees of grids"

TEST(fun_tree, iotaMap) {
  for (std::ptrdiff_t s = 0; s < 110; ++s) {
    auto rs = iotaMap<shared_tree<adt::dummy>>([](auto x) { return int(x); },
                                               adt::irange_t(s));
    static_assert(std::is_same<decltype(rs), shared_tree<int>>::value, "");
    EXPECT_EQ(s, msize(rs));
    if (s > 0)
      EXPECT_EQ(s - 1, last(rs));

    auto rs1 = iotaMap<shared_tree<adt::dummy>>(
        [](auto x, auto y) { return double(x + y); }, adt::irange_t(s), -1);
    static_assert(std::is_same<decltype(rs1), shared_tree<double>>::value, "");
    if (s > 0)
      EXPECT_EQ(s - 2, last(rs1));
  }
}

TEST(fun_tree, fmap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<shared_tree<adt::dummy>>([](auto x) { return int(x); }, s);
  EXPECT_EQ(9, last(xs));

  auto ys = fmap([](auto x, auto y) { return x + y; }, xs, 1);
  EXPECT_EQ(10, last(ys));

  auto zs = fmap2([](auto x, auto y) { return x + y; }, xs, ys);
  EXPECT_EQ(19, last(zs));
}

TEST(fun_tree, boundary) {
  std::ptrdiff_t s = 10;

  auto xs1 = iotaMapMulti<shared_tree<adt::dummy>>([](adt::index_t<1> x) {
    return int(adt::sum(x * x));
  }, adt::range_t<1>(adt::index_t<1>{{s}}));
  typedef typename fun::fun_traits<shared_tree<adt::dummy>>::boundary_dummy
      shared_tree_bnd_dummy;
  typedef
      typename fun::fun_traits<shared_tree_bnd_dummy>::template constructor<int>
          shared_tree_bnd;
  std::array<shared_tree_bnd, 2> bxs1;
  for (std::ptrdiff_t i = 0; i < 2; ++i) {
    bxs1[i] = fun::boundary(xs1, i);
    EXPECT_EQ(fun::msize(bxs1[i]), 1);
  }
  EXPECT_EQ(fun::mextract(bxs1[0]), 0);
  EXPECT_EQ(fun::mextract(bxs1[1]), (s - 1) * (s - 1));

#warning "TODO: test other tree types"
}

#warning "TODO: test boundaryMap"

TEST(fun_tree, fmapStencil) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<shared_tree<adt::dummy>>([](int x) { return x * x; }, s);
  auto ys = fmapStencil(
      [](auto x, auto bdirs, auto bm, auto bp) { return bm - 2 * x + bp; },
      [](auto x, auto i) { return x; }, xs, 0b11, 1, 100);
  auto sum = foldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, ys);
  EXPECT_EQ(20, sum);

  auto x2s = iotaMap<future_tree<adt::dummy>>([](int x) { return x * x; }, s);
  // EXPECT_FALSE(x2s.data.ready());
  auto y2s = fmapStencil(
      [](auto x, auto bdirs, auto bm, auto bp) { return bm - 2 * x + bp; },
      [](auto x, auto i) { return x; }, x2s, 0b11, 1, 100);
  // EXPECT_FALSE(x2s.data.ready());
  // EXPECT_FALSE(y2s.data.ready());
  auto sum2 = foldMap([](auto x) { return x; },
                      [](auto x, auto y) { return x + y; }, 0, y2s);
  // EXPECT_TRUE(x2s.data.ready());
  // EXPECT_TRUE(y2s.data.ready());
  EXPECT_EQ(20, sum2);
}

TEST(fun_tree, foldMap) {
  std::ptrdiff_t s = 10;
  auto xs = iotaMap<shared_tree<adt::dummy>>([](auto x) { return int(x); }, s);
  auto r = foldMap([](auto x) { return x; },
                   [](auto x, auto y) { return x + y; }, 0, xs);
  EXPECT_EQ(s * (s - 1) / 2, r);
}

TEST(fun_tree, monad) {
  auto xs = munit<shared_tree<adt::dummy>>(1);
  auto xss = munit<shared_tree<adt::dummy>>(xs);
  auto ys = mjoin(xss);
  auto zs = mbind([](auto x) { return munit<shared_tree<adt::dummy>>(x); }, ys);
  auto z = mextract(zs);
  EXPECT_EQ(1, z);

  auto z1 = mfoldMap([](auto x) { return x; },
                     [](auto x, auto y) { return x + y; }, 0, zs);
  EXPECT_EQ(1, msize(z1));
  EXPECT_EQ(z, mextract(z1));

  auto ns = mzero<shared_tree<adt::dummy>, int>();
  EXPECT_TRUE(mempty(ns));
  auto ps = mplus(ns, xs, ys, zs);
  EXPECT_EQ(3, msize(ps));
  auto ss = msome<shared_tree<adt::dummy>>(1, 2, 3);
  EXPECT_EQ(3, msize(ss));
  EXPECT_TRUE(mempty(ns));
  EXPECT_FALSE(mempty(ps));
  EXPECT_FALSE(mempty(ss));
}
