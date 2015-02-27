#include <cxx/invoke.hpp>
#include <cxx/serialize.hpp>

#include <cereal/archives/binary.hpp>
#include <gtest/gtest.h>

#include <string>
#include <sstream>

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

int fun() { return 1; }

TEST(cxx_serialize, function) {
  auto orig = &fun;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

struct funobj {
  int operator()() { return 1; }
};

TEST(cxx_serialize, function_object) {
  auto orig = funobj();
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

TEST(cxx_serialize, member_function) {
  auto orig = &funobj::operator();
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ((funobj().*orig)(), (funobj().*copy)());
  EXPECT_EQ(cxx::invoke(orig, funobj()), cxx::invoke(copy, funobj()));
}

struct obj {
  int m;
  obj() : m(1) {}
};

TEST(cxx_serialize, member_object) {
  auto orig = &obj::m;
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
