#include <cxx/invoke.hpp>
#include <cxx/serialize.hpp>

#include <cereal/archives/binary.hpp>
#include <gtest/gtest.h>

#include <sstream>
#include <string>

namespace {
template <typename T> std::string serialize(T &&obj) {
  std::stringstream buf;
  { (cereal::BinaryOutputArchive(buf))(std::forward<T>(obj)); }
  return buf.str();
}

template <typename T> T deserialize(const std::string &str) {
  std::stringstream buf(str);
  T obj;
  { (cereal::BinaryInputArchive(buf))(obj); }
  return obj;
}
} // namespace

namespace {
int fun() { return 1; }
} // namespace

TEST(cxx_serialize, function) {
  auto orig = &fun;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

namespace {
struct funobj {
  int operator()() { return 1; }
};
} // namespace

TEST(cxx_serialize, function_object) {
  auto orig = funobj();
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

namespace {
struct obj {
  int mem;
  int memfun() { return 2; }
  virtual int virtmemfun() { return 3; }
  obj() : mem(1) {}
};
} // namespace

TEST(cxx_serialize, member_function) {
  auto orig = &obj::memfun;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ((obj().*orig)(), (obj().*copy)());
  EXPECT_EQ(cxx::invoke(orig, obj()), cxx::invoke(copy, obj()));
}

TEST(cxx_serialize, virtual_member_function) {
  auto orig = &obj::virtmemfun;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ((obj().*orig)(), (obj().*copy)());
  EXPECT_EQ(cxx::invoke(orig, obj()), cxx::invoke(copy, obj()));
}

TEST(cxx_serialize, member_object) {
  auto orig = &obj::mem;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(obj().*orig, obj().*copy);
  EXPECT_EQ(cxx::invoke(orig, obj()), cxx::invoke(copy, obj()));
}

#if 0
TEST(cxx_serialize, lambda) {
  auto orig0 = [](int x) { return x; };
  auto orig = cxx::lambda_to_function(orig0);
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(cxx::invoke(orig, 3), cxx::invoke(copy, 3));
}
#endif
