#include <adt/maybe.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <tuple>

TEST(adt_maybe, construct) {
  adt::maybe<int>();
  adt::maybe<double>();
  adt::maybe<std::tuple<>>();
}

TEST(adt_maybe, make) {
  adt::make_nothing<int>();
  adt::make_just<int>(1);
  adt::make_nothing<double>();
  adt::make_just<double>(2.0);
  adt::make_nothing<std::tuple<>>();
  adt::make_just<std::tuple<>>();
}

TEST(adt_maybe, assign) {
  auto n = adt::make_nothing<int>();
  EXPECT_TRUE(n.nothing());

  auto j = adt::make_just<int>(1);
  EXPECT_TRUE(j.just());
  EXPECT_EQ(1, j.get_just());

  adt::maybe<int> x = n;
  EXPECT_TRUE(n.nothing());

  x = j;
  EXPECT_TRUE(x.just());
  EXPECT_EQ(1, x.get_just());

  x = adt::make_nothing<int>();
  EXPECT_TRUE(x.nothing());

  x = x;
  EXPECT_TRUE(x.nothing());

  x = adt::make_just<int>(2);
  EXPECT_TRUE(x.just());
  EXPECT_EQ(2, x.get_just());

  x = x;
  EXPECT_TRUE(x.just());
  EXPECT_EQ(2, x.get_just());

  adt::maybe<int> yn(n);
  EXPECT_TRUE(yn.nothing());

  adt::maybe<int> yj(j);
  EXPECT_TRUE(yj.just());
  EXPECT_EQ(1, yj.get_just());

  adt::maybe<int> zn(adt::make_nothing<int>());
  EXPECT_TRUE(zn.nothing());

  adt::maybe<int> zj(adt::make_just<int>(3));
  EXPECT_TRUE(j.just());
  EXPECT_EQ(3, zj.get_just());

  yn.swap(yj);
  EXPECT_TRUE(yn.just());
  EXPECT_EQ(1, yn.get_just());
  EXPECT_TRUE(yj.nothing());

  using std::swap;
  swap(zn, zj);
  EXPECT_TRUE(zn.just());
  EXPECT_EQ(3, zn.get_just());
  EXPECT_TRUE(zj.nothing());
}

TEST(adt_maybe, compare) {
  auto n = adt::make_nothing<int>();
  EXPECT_TRUE(n == n);
  EXPECT_FALSE(n < n);

  auto j1 = adt::make_just<int>(1);
  auto j2 = adt::make_just<int>(2);
  EXPECT_TRUE(j1 == j1);
  EXPECT_FALSE(j1 == j2);
  EXPECT_FALSE(j1 < j1);
  EXPECT_TRUE(j1 < j2);

  EXPECT_FALSE(n == j1);
  EXPECT_TRUE(n < j1);

  EXPECT_FALSE(adt::make_nothing<std::tuple<>>() ==
               adt::make_just<std::tuple<>>());
  EXPECT_TRUE(adt::make_nothing<std::tuple<>>() <
              adt::make_just<std::tuple<>>());
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

TEST(adt_maybe, serialize) {
  test_serialize(adt::make_nothing<int>());
  test_serialize(adt::make_just<int>(1));
  test_serialize(adt::make_nothing<std::string>());
  test_serialize(adt::make_just<std::string>("hello"));
  test_serialize(adt::make_nothing<std::tuple<>>());
  test_serialize(adt::make_just<std::tuple<>>());
}
