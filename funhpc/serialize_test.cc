#include <cxx/invoke>
#include <funhpc/serialize>

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

TEST(funhpc_rexec, serialize_function) {
  auto orig = &fun;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

struct funobj {
  int operator()() { return 1; }
};

TEST(funhpc_rexec, serialize_function_object) {
  auto orig = funobj();
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(orig(), copy());
  EXPECT_EQ(cxx::invoke(orig), cxx::invoke(copy));
}

TEST(funhpc_rexec, serialize_member_function) {
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

TEST(funhpc_rexec, serialize_member_object) {
  auto orig = &obj::m;
  auto buf = serialize(orig);
  auto copy = deserialize<decltype(orig)>(buf);
  EXPECT_EQ(obj().*orig, obj().*copy);
  EXPECT_EQ(cxx::invoke(orig, obj()), cxx::invoke(copy, obj()));
}
