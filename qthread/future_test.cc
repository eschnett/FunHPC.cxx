#include <qthread.h>

#include <qthread/future>

#include <gtest/gtest.h>

using namespace qthread;

namespace {

int fi(int x) { return x; }
void fv(int) {}

template <typename T> void test_future() {
  std::cerr << "test_future.0\n";
  qthread_initialize();
  std::cerr << "test_future.1\n";
  future<T> f0;
  future<T> f1(std::move(f0));
  f0 = std::move(f1);
  swap(f0, f1);
  EXPECT_FALSE(f0.valid());
  EXPECT_FALSE(f1.valid());
  // std::cerr << "test_future.f2.0\n";
  // auto f2 = make_ready_future(1);
  // std::cerr << "test_future.f2.9\n";
  // EXPECT_TRUE(f2.is_ready());
  // f0 = std::move(f2);
  // swap(f0,f2);
  // f2.wait();
  // EXPECT_EQ(f2.get(), 1);
  // f0.share();
  // f2.share();
  std::cerr << "test_future.8\n";
  qthread_finalize();
  std::cerr << "test_future.9\n";
}

template <typename T> void test_shared_future() {
  shared_future<T> f;
  shared_future<T> f1(std::move(f));
  shared_future<T> f2(f1);
  shared_future<T> f3((future<T>()));
  f = std::move(f1);
  f = future<T>();
  f = f1;
  swap(f, f1);
  f.valid();
  f.is_ready();
  f.wait();
  f.get();
}

template <typename T> void test_promise(T value) {
  promise<T> p;
  promise<T> p1(std::move(p));
  p = std::move(p1);
  swap(p, p1);
  p.set_value(value);
  p.get_future();
}

template <typename R, typename... Args, typename F>
void test_packaged_task(F &&f, Args... args) {
  packaged_task<R(Args...)> t;
  packaged_task<R(Args...)> t1(std::forward<F>(f));
  packaged_task<R(Args...)> t2(std::move(t));
  t = std::move(t1);
  t.valid();
  swap(t, t1);
  t.get_future();
  t(args...);
  t.reset();
}
}

TEST(qthread_future, future) {
  test_future<int>();
  test_future<int &>();
  test_future<const int &>();
  test_future<int (*)(int)>();
  test_future<int(&)(int)>();
}

TEST(qthread_future, make_ready_future) {
  int i{};
  const int ci{};
  make_ready_future(1);
  make_ready_future(i);
  make_ready_future(ci);
  make_ready_future();
}

TEST(qthread_future, shared_future) {
  test_shared_future<int>();
  test_shared_future<int &>();
  test_shared_future<const int &>();
  test_shared_future<int (*)(int)>();
  test_shared_future<int(&)(int)>();
}

TEST(qthread_future, promise) {
  int i{};
  test_promise<int>(i);
  test_promise<int &>(i);
  test_promise<const int &>(i);
  test_promise<int (*)(int)>(fi);
  test_promise<int(&)(int)>(fi);
}

TEST(qthread_future, packaged_task) {
  test_packaged_task<int, int>(fi, 0);
  test_packaged_task<int, int>(&fi, 0);
  test_packaged_task<int, int>(std::function<int(int)>(fi), 0);
  test_packaged_task<int, int>([](int x) { return x; }, 0);
  test_packaged_task<void, int>(fv, 0);
  test_packaged_task<void, int>(&fv, 0);
  test_packaged_task<void, int>(std::function<void(int)>(fv), 0);
  test_packaged_task<void, int>([](int) {}, 0);
}

TEST(qthread_future, async) {
  async(fi, 1);
  async(fv, 1);
  async([]() {});
  async([](int x) { return x; }, 1);
  async([](int) {}, 1);

  async(launch::async, fi, 1);
  async(launch::deferred, fi, 1);
  async(launch::sync, fi, 1);
  async(launch::detached, fi, 1);
}
