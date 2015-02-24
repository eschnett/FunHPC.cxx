#include <adt/either.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <tuple>
#include <utility>

TEST(adt_either, construct) {
  adt::either<int, double>();
  adt::either<int, int>();
  adt::either<std::tuple<>, int>();
}

TEST(adt_either, make) {
  adt::make_left<int, double>(1);
  adt::make_right<int, double>(2.0);
  adt::make_left<int, int>(3);
  adt::make_right<int, int>(4);
  adt::make_left<std::tuple<>, int>();
  adt::make_right<std::tuple<>, int>(5);
}

TEST(adt_either, assign) {
  auto l = adt::make_left<int, double>(1);
  EXPECT_TRUE(l.left());
  EXPECT_EQ(1, l.get_left());

  auto r = adt::make_right<int, double>(2.0);
  EXPECT_TRUE(r.right());
  EXPECT_EQ(2.0, r.get_right());

  adt::either<int, double> x = l;
  EXPECT_TRUE(x.left());
  EXPECT_EQ(1, x.get_left());

  x = r;
  EXPECT_TRUE(x.right());
  EXPECT_EQ(2.0, x.get_right());

  x = adt::make_left<int, double>(3);
  EXPECT_TRUE(x.left());
  EXPECT_EQ(3, x.get_left());

  x = x;
  EXPECT_TRUE(x.left());
  EXPECT_EQ(3, x.get_left());

  x = adt::make_right<int, double>(4.0);
  EXPECT_TRUE(x.right());
  EXPECT_EQ(4.0, x.get_right());

  x = x;
  EXPECT_TRUE(x.right());
  EXPECT_EQ(4.0, x.get_right());

  adt::either<int, double> yl(l);
  EXPECT_TRUE(yl.left());
  EXPECT_EQ(1, yl.get_left());

  adt::either<int, double> yr(r);
  EXPECT_TRUE(yr.right());
  EXPECT_EQ(2.0, yr.get_right());

  adt::either<int, double> zl(adt::make_left<int, double>(5));
  EXPECT_TRUE(zl.left());
  EXPECT_EQ(5, zl.get_left());

  adt::either<int, double> zr(adt::make_right<int, double>(6.0));
  EXPECT_TRUE(zr.right());
  EXPECT_EQ(6.0, zr.get_right());

  yl.swap(yr);
  EXPECT_TRUE(yl.right());
  EXPECT_EQ(2.0, yl.get_right());
  EXPECT_TRUE(yr.left());
  EXPECT_EQ(1, yr.get_left());

  using std::swap;
  swap(zl, zr);
  EXPECT_TRUE(zl.right());
  EXPECT_EQ(6.0, zl.get_right());
  EXPECT_TRUE(zr.left());
  EXPECT_EQ(5, zr.get_left());
}

TEST(adt_either, compare) {
  auto l1 = adt::make_left<int, double>(1);
  auto l2 = adt::make_left<int, double>(2);
  EXPECT_TRUE(l1 == l1);
  EXPECT_FALSE(l1 == l2);
  EXPECT_FALSE(l1 < l1);
  EXPECT_TRUE(l1 < l2);

  auto r3 = adt::make_right<int, double>(3.0);
  auto r4 = adt::make_right<int, double>(4.0);
  EXPECT_TRUE(r3 == r3);
  EXPECT_FALSE(r3 == r4);
  EXPECT_FALSE(r3 < r3);
  EXPECT_TRUE(r3 < r4);

  EXPECT_FALSE(l1 == r3);
  EXPECT_TRUE(l1 < r3);
}

namespace {
template <typename T> void test_serialize(const T &x) {
  std::string ar;
  {
    std::stringstream buf;
    { (cereal::BinaryOutputArchive(buf))(x); }
    ar = buf.str();
  }
  T y;
  {
    std::stringstream buf(ar);
    (cereal::BinaryInputArchive(buf))(y);
  }
  EXPECT_EQ(x, y);
}
}

TEST(adt_either, serialize) {
  test_serialize(adt::make_left<int, double>(1));
  test_serialize(adt::make_right<int, double>(2.0));
  test_serialize(adt::make_left<std::string, std::tuple<>>("hello"));
  test_serialize(adt::make_right<std::string, std::tuple<>>());
}
