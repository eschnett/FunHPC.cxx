#include <cxx/invoke>

#include <gtest/gtest.h>

#include <functional>

using namespace cxx;

namespace {

int f0(int x) { return x; }

struct s {
  int d0;
  const int d1;
  s() : d0(1), d1(2) {}
  int m0(int x) { return x; }
  int m1(int x) const { return x; }
};

struct o0 {
  int operator()(int x) { return x; }
};
struct o1 {
  int operator()(int x) const { return x; }
};
}

TEST(cxx_invoke, invoke) {
  EXPECT_EQ(invoke(f0, 1), 1);
  EXPECT_EQ(invoke(&f0, 1), 1);

  s s0{};
  const s s1{};
  EXPECT_EQ(invoke(&s::m0, s0, 1), 1);
  EXPECT_EQ(invoke(&s::m1, s0, 1), 1);
  EXPECT_EQ(invoke(&s::m1, s1, 1), 1);
  EXPECT_EQ(invoke(&s::m0, &s0, 1), 1);
  EXPECT_EQ(invoke(&s::m1, &s0, 1), 1);
  EXPECT_EQ(invoke(&s::m1, &s1, 1), 1);

  EXPECT_EQ(invoke(&s::d0, s0), 1);
  EXPECT_EQ(invoke(&s::d1, s0), 2);
  EXPECT_EQ(invoke(&s::d0, s1), 1);
  EXPECT_EQ(invoke(&s::d1, s1), 2);
  EXPECT_EQ(invoke(&s::d0, s0) = 3, 3);

  o0 fo0{};
  o1 fo1{};
  EXPECT_EQ(invoke(fo0, 1), 1);
  EXPECT_EQ(invoke(fo1, 1), 1);

  EXPECT_EQ(invoke(std::bind(f0, std::placeholders::_1), 1), 1);
  EXPECT_EQ(invoke(std::bind(f0, 1)), 1);

  auto rl = invoke([](int x) { return x; }, 1);
  EXPECT_EQ(rl, 1);
  auto rml = invoke([](int x) mutable { return x; }, 1);
  EXPECT_EQ(rml, 1);
}

TEST(cxx_invoke, invoke_of_t) {
  typedef invoke_of_t<int (*)(int), int> a0;
  typedef invoke_of_t<int (*)(int), const int &> a1;
  typedef invoke_of_t<int (*)(int), int &> a2;
  typedef invoke_of_t<int (*)(int), int &&> a3;

  typedef invoke_of_t<int(&)(int), int> b0;
  typedef invoke_of_t<int(&)(int), const int &> b1;
  typedef invoke_of_t<int(&)(int), int &> b2;
  typedef invoke_of_t<int(&)(int), int && > b3;

  auto c0 = [](int) {};
  typedef invoke_of_t<decltype(c0), int> c1;
}
