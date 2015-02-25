#include <fun/shared_ptr.hpp>
#include <fun/vector.hpp>

#include <fun/nested.hpp>
#include <fun/fun.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace fun;

namespace {
template <typename T> using vector1 = std::vector<T>;
template <typename T>
using shared_vector = adt::nested<std::shared_ptr, vector1, T>;
}

TEST(fun_fun, size) {
  auto xs = msome<vector1, char>('h', 'e', 'l', 'l', 'o');
  EXPECT_FALSE(empty(xs));
  EXPECT_EQ(5, size(xs));
}

TEST(fun_fun, convert) {
  auto xs = msome<vector1, char>('h', 'e', 'l', 'l', 'o');
  auto ys = convert<shared_vector>(xs);
  auto zs = convert<vector1>(ys);
  EXPECT_EQ(xs, zs);
}

TEST(fun_fun, to_string) {
  auto xs = msome<vector1, char>('h', 'e', 'l', 'l', 'o');
  auto s = to_string(xs);
  EXPECT_EQ("hello", s);
}
