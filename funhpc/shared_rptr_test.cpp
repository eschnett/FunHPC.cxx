#include <funhpc/async.hpp>
#include <funhpc/shared_rptr.hpp>

#include <cereal/access.hpp>
#include <gtest/gtest.h>

using namespace funhpc;

namespace {
struct s {
  int x;
};

int f(int x) { return x; }
}

TEST(funhpc_shared_rptr, empty) {
  auto pi = shared_rptr<int>();
  auto ps = shared_rptr<s>();
  auto pf = shared_rptr<int (*)(int)>();
  auto pp = shared_rptr<shared_rptr<int>>();
  EXPECT_FALSE(bool(pi));
  EXPECT_FALSE(bool(ps));
  EXPECT_FALSE(bool(pf));
  EXPECT_FALSE(bool(pp));
}

TEST(funhpc_shared_rptr, local) {
  auto pi = make_shared_rptr<int>(1);
  auto ps = make_shared_rptr<s>(s());
  auto pf = make_shared_rptr<int (*)(int)>(f);
  auto pp = make_shared_rptr<shared_rptr<int>>(make_shared_rptr<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  EXPECT_EQ(1, *pi);
  EXPECT_EQ(1, **pp);
}

namespace {
template <typename T> shared_rptr<T> bounce(const shared_rptr<T> &p) {
  EXPECT_TRUE(bool(p));
  if (rank() > 0)
    EXPECT_FALSE(p.local());
  return p;
}
}

TEST(funhpc_shared_rptr, remote) {
  auto pi = make_shared_rptr<int>(1);
  auto ps = make_shared_rptr<s>(s());
  auto pf = make_shared_rptr<int (*)(int)>(f);
  auto pp = make_shared_rptr<shared_rptr<int>>(make_shared_rptr<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  auto fi = async(rlaunch::async, 1 % size(), bounce<int>, pi);
  auto fs = async(rlaunch::async, 1 % size(), bounce<s>, ps);
  auto ff = async(rlaunch::async, 1 % size(), bounce<int (*)(int)>, pf);
  auto fp = async(rlaunch::async, 1 % size(), bounce<shared_rptr<int>>, pp);
  EXPECT_EQ(pi.get_shared_ptr().get(), fi.get().get_shared_ptr().get());
  EXPECT_EQ(ps.get_shared_ptr().get(), fs.get().get_shared_ptr().get());
  EXPECT_EQ(pf.get_shared_ptr().get(), ff.get().get_shared_ptr().get());
  EXPECT_EQ(pp.get_shared_ptr().get(), fp.get().get_shared_ptr().get());
}

namespace {
struct t {
  template <typename Archive> void serialize(Archive &ar) { ar(x); }
  int x;
  bool operator==(const t &other) const { return x == other.x; }
};

template <typename T> void check(const shared_rptr<T> &p, const T &x) {
  EXPECT_TRUE(bool(p));
  auto fp = make_local_shared_ptr(p);
  static_assert(
      std::is_same<decltype(fp), qthread::future<std::shared_ptr<T>>>::value,
      "");
  EXPECT_EQ(x, *fp.get());
}
}

TEST(funhpc_shared_rptr, get) {
  auto pi = make_shared_rptr<int>(1);
  auto ps = make_shared_rptr<t>(t{3});
  auto pf = make_shared_rptr<int (*)(int)>(f);
  auto pp = make_shared_rptr<shared_rptr<int>>(make_shared_rptr<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  async(rlaunch::sync, 1 % size(), check<int>, pi, *pi);
  async(rlaunch::sync, 1 % size(), check<t>, ps, *ps);
  async(rlaunch::sync, 1 % size(), check<int (*)(int)>, pf, *pf);
  async(rlaunch::sync, 1 % size(), check<shared_rptr<int>>, pp, *pp);
}
