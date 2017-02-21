#include <fun/par_decl.hpp>

#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <fun/par_impl.hpp>

#include <gtest/gtest.h>

template <typename T>
using shared_vector =
    adt::par<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;

TEST(fun_par, iotaMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_FALSE(xs.data.left());
  EXPECT_EQ(10, fun::msize(xs.data.get_right()));
}

TEST(fun_par, dump) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  std::string str(fun::dump(xs));
  EXPECT_EQ("par{right{[0,1,2,3,4,5,6,7,8,9,]}}", str);
}

TEST(fun_par, fmap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::fmap([](int x) { return x + 1; }, xs);
  EXPECT_FALSE(rs.data.left());
  EXPECT_EQ(10, fun::msize(rs.data.get_right()));
  EXPECT_EQ(1, rs.data.get_right().front());
  EXPECT_EQ(10, rs.data.get_right().back());
}

TEST(fun_par, fmap2) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return -x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::fmap2([](int x, int y) { return x + y; }, xs, ys);
  EXPECT_FALSE(rs.data.left());
  EXPECT_EQ(10, fun::msize(rs.data.get_right()));
  EXPECT_EQ(0, rs.data.get_right().front());
  EXPECT_EQ(0, rs.data.get_right().back());
}

TEST(fun_par, fmap3) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x + 1; },
                                                    adt::irange_t(0, 10));
  auto zs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return 2 * x; },
                                                    adt::irange_t(0, 10));
  auto rs =
      fun::fmap3([](int x, int y, int z) { return x + y - z; }, xs, ys, zs);
  EXPECT_FALSE(rs.data.left());
  EXPECT_EQ(10, fun::msize(rs.data.get_right()));
  EXPECT_EQ(1, rs.data.get_right().front());
  EXPECT_EQ(1, rs.data.get_right().back());
}

TEST(fun_par, head) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_EQ(0, fun::head(xs));
  EXPECT_EQ(9, fun::last(xs));
}

TEST(fun_par, munit) {
  auto xs = fun::munit<shared_vector<adt::dummy>>(42);
  EXPECT_TRUE(xs.data.left());
  EXPECT_EQ(42, *xs.data.get_left());
}

TEST(fun_par, foldMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto r = fun::foldMap([](int x) { return x; },
                        [](int x, int y) { return x + y; }, 0, xs);
  EXPECT_EQ(45, r);
}

TEST(fun_par, foldMap2) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x + 1; },
                                                    adt::irange_t(0, 10));
  auto r = fun::foldMap2([](int x, int y) { return x - y; },
                         [](int x, int y) { return x + y; }, 0, xs, ys);
  EXPECT_EQ(-10, r);
}

TEST(fun_par, mextract) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_EQ(0, fun::mextract(xs));
}

TEST(fun_par, mfoldMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::mfoldMap([](int x) { return x; },
                          [](int x, int y) { return x + y; }, 0, xs);
  EXPECT_TRUE(rs.data.left());
  EXPECT_EQ(45, *rs.data.get_left());
}

TEST(fun_par, mzero) {
  auto xs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_TRUE(xs.data.left());
  EXPECT_FALSE(bool(xs.data.get_left()));
}

#if 0
TEST(fun_par, mplus) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::mplus(xs, xs, xs);
  EXPECT_TRUE( bool(rs.data.first));
  EXPECT_EQ(29, rs.data.second.size());
}
#endif

TEST(fun_par, msome) {
  auto xs = fun::msome<shared_vector<adt::dummy>>(1, 2, 4, 8);
  EXPECT_FALSE(xs.data.left());
  EXPECT_EQ(4, xs.data.get_right().size());
  EXPECT_EQ(1, xs.data.get_right().front());
  EXPECT_EQ(8, xs.data.get_right().back());
}

TEST(fun_par, mempty) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::munit<shared_vector<adt::dummy>>(42);
  auto zs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_FALSE(fun::mempty(xs));
  EXPECT_FALSE(fun::mempty(ys));
  EXPECT_TRUE(fun::mempty(zs));
}

TEST(fun_par, msize) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::munit<shared_vector<adt::dummy>>(42);
  auto zs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_EQ(10, fun::msize(xs));
  EXPECT_EQ(1, fun::msize(ys));
  EXPECT_EQ(0, fun::msize(zs));
}
