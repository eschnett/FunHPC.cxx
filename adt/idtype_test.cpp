#include <adt/idtype.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <tuple>

TEST(adt_idtype, construct) {
  adt::idtype<int>(1);
  adt::idtype<double>(2.0);
  adt::idtype<std::tuple<>>(std::tuple<>());
}

TEST(adt_idtype, assign) {
  auto j = adt::idtype<int>(1);
  EXPECT_EQ(1, j.get());

  adt::idtype<int> x;
  x = j;
  EXPECT_EQ(1, x.get());

  x = adt::idtype<int>(2);
  EXPECT_EQ(2, x.get());

  x = x;
  EXPECT_EQ(2, x.get());

  adt::idtype<int> yj(j);
  EXPECT_EQ(1, yj.get());

  adt::idtype<int> zj(adt::idtype<int>(3));
  EXPECT_EQ(3, zj.get());

  yj.swap(zj);
  EXPECT_EQ(1, zj.get());
  EXPECT_EQ(3, yj.get());

  using std::swap;
  swap(zj, yj);
  EXPECT_EQ(3, zj.get());
  EXPECT_EQ(1, yj.get());
}

TEST(adt_idtype, compare) {
  auto j1 = adt::idtype<int>(1);
  auto j2 = adt::idtype<int>(2);
  EXPECT_TRUE(j1 == j1);
  EXPECT_FALSE(j1 == j2);
  EXPECT_FALSE(j1 < j1);
  EXPECT_TRUE(j1 < j2);
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
} // namespace

TEST(adt_idtype, serialize) {
  test_serialize(adt::idtype<int>(1));
  test_serialize(adt::idtype<std::string>("hello"));
  test_serialize(adt::idtype<std::tuple<>>(std::tuple<>()));
}
