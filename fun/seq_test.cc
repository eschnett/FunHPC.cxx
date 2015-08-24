#include <fun/seq_decl.hpp>

#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <fun/seq_impl.hpp>

#include <gtest/gtest.h>

template <typename T>
using shared_vector =
    adt::seq<std::shared_ptr<adt::dummy>, std::vector<adt::dummy>, T>;

TEST(fun_seq, iotaMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_TRUE(bool(xs.data.first));
  EXPECT_EQ(0, *xs.data.first);
  EXPECT_EQ(9, xs.data.second.size());
  EXPECT_EQ(1, xs.data.second.front());
  EXPECT_EQ(9, xs.data.second.back());
}

TEST(fun_seq, dump) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  std::string str(fun::dump(xs));
  EXPECT_EQ("seq{first{[0,]},second{[1,2,3,4,5,6,7,8,9,]}}", str);
}

TEST(fun_seq, fmap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::fmap([](int x) { return x + 1; }, xs);
  EXPECT_TRUE(bool(rs.data.first));
  EXPECT_EQ(1, *rs.data.first);
  EXPECT_EQ(9, rs.data.second.size());
  EXPECT_EQ(2, rs.data.second.front());
  EXPECT_EQ(10, rs.data.second.back());
}

TEST(fun_seq, fmap2) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return -x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::fmap2([](int x, int y) { return x + y; }, xs, ys);
  EXPECT_TRUE(bool(rs.data.first));
  EXPECT_EQ(0, *rs.data.first);
  EXPECT_EQ(9, rs.data.second.size());
  EXPECT_EQ(0, rs.data.second.front());
  EXPECT_EQ(0, rs.data.second.back());
}

TEST(fun_seq, fmap3) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x + 1; },
                                                    adt::irange_t(0, 10));
  auto zs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return 2 * x; },
                                                    adt::irange_t(0, 10));
  auto rs =
      fun::fmap3([](int x, int y, int z) { return x + y - z; }, xs, ys, zs);
  EXPECT_TRUE(bool(rs.data.first));
  EXPECT_EQ(1, *rs.data.first);
  EXPECT_EQ(9, rs.data.second.size());
  EXPECT_EQ(1, rs.data.second.front());
  EXPECT_EQ(1, rs.data.second.back());
}

TEST(fun_seq, head) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_EQ(0, fun::head(xs));
  EXPECT_EQ(9, fun::last(xs));
}

TEST(fun_seq, foldMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto r = fun::foldMap([](int x) { return x; },
                        [](int x, int y) { return x + y; }, 0, xs);
  EXPECT_EQ(45, r);
}

TEST(fun_seq, foldMap2) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x + 1; },
                                                    adt::irange_t(0, 10));
  auto r = fun::foldMap2([](int x, int y) { return x - y; },
                         [](int x, int y) { return x + y; }, 0, xs, ys);
  EXPECT_EQ(-10, r);
}

TEST(fun_seq, munit) {
  auto xs = fun::munit<shared_vector<adt::dummy>>(42);
  EXPECT_TRUE(bool(xs.data.first));
  EXPECT_EQ(42, *xs.data.first);
  EXPECT_EQ(0, xs.data.second.size());
}

TEST(fun_seq, mextract) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  EXPECT_EQ(0, fun::mextract(xs));
}

TEST(fun_seq, mfoldMap) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::mfoldMap([](int x) { return x; },
                          [](int x, int y) { return x + y; }, 0, xs);
  EXPECT_TRUE(bool(rs.data.first));
  EXPECT_EQ(45, *rs.data.first);
  EXPECT_EQ(0, rs.data.second.size());
}

TEST(fun_seq, mzero) {
  auto xs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_FALSE(bool(xs.data.first));
  EXPECT_EQ(0, xs.data.second.size());
}

#if 0
TEST(fun_seq, mplus) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto rs = fun::mplus(xs, xs, xs);
  EXPECT_TRUE(bool(rs.data.first));
  EXPECT_EQ(29, rs.data.second.size());
}

TEST(fun_seq, msome) {
  auto xs = fun::msome<shared_vector<adt::dummy>>(1, 2, 4, 8);
  EXPECT_TRUE(bool(xs.data.first));
  EXPECT_EQ(1, *xs.data.first);
  EXPECT_EQ(3, xs.data.second.size());
  EXPECT_EQ(2, xs.data.second.front());
  EXPECT_EQ(8, xs.data.second.back());
}
#endif

TEST(fun_seq, mempty) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::munit<shared_vector<adt::dummy>>(42);
  auto zs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_FALSE(fun::mempty(xs));
  EXPECT_FALSE(fun::mempty(ys));
  EXPECT_TRUE(fun::mempty(zs));
}

TEST(fun_seq, msize) {
  auto xs = fun::iotaMap<shared_vector<adt::dummy>>([](int x) { return x; },
                                                    adt::irange_t(0, 10));
  auto ys = fun::munit<shared_vector<adt::dummy>>(42);
  auto zs = fun::mzero<shared_vector<adt::dummy>, int>();
  EXPECT_EQ(10, fun::msize(xs));
  EXPECT_EQ(1, fun::msize(ys));
  EXPECT_EQ(0, fun::msize(zs));
}
