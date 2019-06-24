#include <funhpc/proxy.hpp>

#include <funhpc/async.hpp>

#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>
#include <gtest/gtest.h>

using namespace funhpc;

namespace {
struct s {
  template <typename Archive> void serialize(Archive &ar) { ar(x); }
  int x;
};
s make_s() { return {int(rank())}; }

int f(int x) { return x; }
} // namespace

TEST(funhpc_proxy, empty) {
  auto pi = proxy<int>();
  auto ps = proxy<s>();
  auto pf = proxy<int (*)(int)>();
  auto pp = proxy<proxy<int>>();
  EXPECT_FALSE(bool(pi));
  EXPECT_FALSE(bool(ps));
  EXPECT_FALSE(bool(pf));
  EXPECT_FALSE(bool(pp));
}

TEST(funhpc_proxy, make_local) {
  auto pi = make_local_proxy<int>(1);
  auto ps = make_local_proxy<s>(s());
  auto pf = make_local_proxy<int (*)(int)>(f);
  auto pp = make_local_proxy<proxy<int>>(make_local_proxy<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  EXPECT_TRUE(pi.local());
  EXPECT_TRUE(ps.local());
  EXPECT_TRUE(pf.local());
  EXPECT_TRUE(pp.local());
  EXPECT_EQ(1, *pi);
  EXPECT_EQ(1, **pp);
}

TEST(funhpc_proxy, make_remote) {
  auto p = 1 % size();
  auto pi = make_remote_proxy<int>(p, 1);
  auto ps = make_remote_proxy<s>(p, s());
  auto pf = make_remote_proxy<int (*)(int)>(p, f);
  auto pp = make_remote_proxy<proxy<int>>(p, make_remote_proxy<int>(p, 1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  if (p != rank()) {
    EXPECT_FALSE(pi.local());
    EXPECT_FALSE(ps.local());
    EXPECT_FALSE(pf.local());
    EXPECT_FALSE(pp.local());
  }
  auto pil = pi.make_local();
  auto psl = ps.make_local();
  auto pfl = pf.make_local();
  auto ppl = pp.make_local();
  EXPECT_TRUE(bool(pil));
  EXPECT_TRUE(bool(psl));
  EXPECT_TRUE(bool(pfl));
  EXPECT_TRUE(bool(ppl));
  EXPECT_TRUE(pil.local());
  EXPECT_TRUE(psl.local());
  EXPECT_TRUE(pfl.local());
  EXPECT_TRUE(ppl.local());
  EXPECT_EQ(1, *pil);
  auto ppll = ppl->make_local();
  EXPECT_TRUE(bool(ppll));
  EXPECT_TRUE(ppll.local());
  EXPECT_EQ(1, *ppll);
}

TEST(funhpc_proxy, remote) {
  auto p = 1 % size();
  auto pi = remote(p, f, 1);
  auto ps = remote(p, make_s);
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  if (p != rank()) {
    EXPECT_FALSE(pi.local());
    EXPECT_FALSE(ps.local());
  }
  auto pil = pi.make_local();
  auto psl = ps.make_local();
  EXPECT_TRUE(bool(pil));
  EXPECT_TRUE(bool(psl));
  EXPECT_TRUE(pil.local());
  EXPECT_TRUE(psl.local());
  EXPECT_EQ(1, *pil);
  EXPECT_EQ(p, psl->x);
}

namespace {
struct s0 {
  int i;
  template <typename Archive> void serialize(Archive &ar) { ar(i); }
};
struct s1 {
  std::vector<proxy<s0>> pis;
  s1() : pis(10) {}
  template <typename Archive> void serialize(Archive &ar) { ar(pis); }
};
s1 localize(const proxy<s1> &oldp) {
  s1 res;
  const auto oldp_local = oldp.make_local();
  const auto &old = *oldp_local;
  for (int n = 0; n < int(old.pis.size()); ++n)
    res.pis[n] = old.pis[n].make_local();
  return res;
}
int getvalue(const std::vector<proxy<s1>> &all_p1) {
  const auto &p1 = all_p1[rank()];
  EXPECT_TRUE(bool(p1));
  EXPECT_TRUE(bool(p1.local()));
  const auto &pi = p1->pis[p1->pis.size() / 2];
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(pi.local()));
  return pi->i;
}
} // namespace

TEST(funhpc_proxy, nested) {
  auto p1 = make_local_proxy<s1>();
  for (int n = 0; n < int(p1->pis.size()); ++n)
    p1->pis[n] = make_local_proxy<s0>(s0{n});
  std::vector<proxy<s1>> all_p1(size());
  for (int p = 0; p < size(); ++p)
    all_p1[p] = remote(p, localize, p1);
  auto i = async(rlaunch::sync, 1 % size(), getvalue, all_p1).get();
  EXPECT_EQ(5, i);
}
