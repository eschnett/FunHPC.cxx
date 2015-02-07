#include <cxx/apply>
#include <cxx/invoke>

#include <gtest/gtest.h>

#include <cassert>
#include <tuple>
#include <type_traits>

using namespace cxx;

namespace {

int f(int x) { return x; }
int f2(int x, double y) { return x; }
void fv(int) {}

int fp(int (*f)(int), int x) { return invoke(f, x); }
int fr(int (&f)(int), int x) { return invoke(f, x); }

template <typename F, typename... Args> struct task {
  static_assert(std::is_same<F, std::decay_t<F>>::value, "");
  static_assert(std::is_same<std::tuple<Args...>,
                             std::tuple<std::decay_t<Args>...>>::value,
                "");
  F f;
  std::tuple<Args...> args;
  template <typename F1, typename... Args1>
  task(F1 &&f, Args1 &&... args)
      : f(std::forward<F1>(f)),
        args(std::make_tuple(std::forward<Args1>(args)...)) {}
  typedef decltype(apply(f, args)) result_type;
  result_type operator()() const { return apply(f, args); }
};
template <typename F, typename... Args> auto make_task(F &&f, Args &&... args) {
  return task<std::decay_t<F>, std::decay_t<Args>...>(
      std::forward<F>(f), std::forward<Args>(args)...);
}
}

TEST(cxx_apply, apply) {
  std::tuple<int> t(1);
  std::tuple<int, double> t2(1, 2.0);
  EXPECT_EQ(apply(f, t), 1);
  EXPECT_EQ(apply(f2, t2), 1);
  apply(fv, t);
  std::tuple<int (*)(int), int> tp(&f, 1);
  std::tuple<int(&)(int), int> tr(f, 1);
  EXPECT_EQ(apply(fp, tp), 1);
  EXPECT_EQ(apply(fr, tr), 1);
}

TEST(cxx_apply, make_task) {
  make_task([] {})();
  EXPECT_EQ(make_task(f, 1)(), 1);
  EXPECT_EQ(make_task(f2, 1, 2.0)(), 1);
  make_task(fv, 1)();
  EXPECT_EQ(make_task(fp, f, 1)(), 1);
  EXPECT_EQ(make_task(fp, &f, 1)(), 1);
}

TEST(cxx_apply, make_task_fancy) {
  EXPECT_EQ((make_task<int (*)(int), int>(f, 1)()), 1);
  // make_task(make_task<int (*const &)(int), const int &>, f, 1)();
  EXPECT_EQ(make_task(make_task(f, 1))(), 1);
  auto rfp =
      make_task([](int (*f)(int), int x) { return make_task(f, x); }, f, 1)()();
  EXPECT_EQ(rfp, 1);
  auto rfr = make_task([](int (*f)(int), int x) { return make_task(f, x); }, &f,
                       1)()();
  EXPECT_EQ(rfr, 1);
}
