#include <cxx/utility.hpp>

#include <gtest/gtest.h>

#include <future>
#include <thread>

using namespace std;

namespace {
int fi(int x) { return x; }
void fv(int) {}

template <typename T> void test_future(T value) {
  future<T> f0;
  future<T> f1(std::move(f0));
  f0 = std::move(f1);
  swap(f0, f1);
  EXPECT_FALSE(f0.valid());
  EXPECT_FALSE(f1.valid());

  promise<T> p2;
  p2.set_value(value);
  auto f2 = p2.get_future();
  EXPECT_TRUE(f2.valid());
  EXPECT_TRUE(f2.wait_for(std::chrono::seconds(0)) == future_status::ready);
  f0 = std::move(f2);
  swap(f0, f2);
  f2.wait();
  typedef std::decay_t<T> decay_T; // avoid function references
  EXPECT_EQ(decay_T(value), decay_T(f2.get()));

  promise<T> p3;
  p3.set_value(value);
  auto f3 = p3.get_future();
  EXPECT_TRUE(f3.wait_for(std::chrono::seconds(0)) == future_status::ready);
  f3.share();
}
} // namespace

TEST(std_future, future) {
  int i{};
  test_future<int>(i);
  test_future<int &>(i);
  test_future<const int &>(i);
  test_future<int (*)(int)>(fi);
  test_future<int (&)(int)>(fi);
}

// TEST(std_future, make_ready_future) {
//   int i{};
//   const int ci{};
//   auto f1 = make_ready_future(1);
//   EXPECT_EQ(1,f1.get());
//   auto fi = make_ready_future(i);
//   EXPECT_EQ(1,fi.get());
//   auto fci = make_ready_future(ci);
//   EXPECT_EQ(ci,fci.get());
//   auto f4 = make_ready_future();
//   static_assert(std::is_same<decltype(f4), future<void>>::value, "");
// }

namespace {
template <typename T> void test_shared_future(T value) {
  shared_future<T> f0;
  shared_future<T> f1(std::move(f0));
  shared_future<T> f2(f1);
  shared_future<T> f3((future<T>()));
  f0 = std::move(f1);
  f0 = future<T>();
  f0 = f1;
  swap(f0, f1);
  EXPECT_FALSE(f0.valid());
  EXPECT_FALSE(f1.valid());
  EXPECT_FALSE(f2.valid());
  EXPECT_FALSE(f3.valid());

  promise<T> p4;
  p4.set_value(value);
  auto f4 = p4.get_future().share();
  EXPECT_TRUE(f4.valid());
  EXPECT_TRUE(f4.wait_for(std::chrono::seconds(0)) == future_status::ready);
  f4.wait();
  typedef std::decay_t<T> decay_T; // avoid function references
  EXPECT_EQ(decay_T(value), decay_T(f4.get()));
  EXPECT_EQ(decay_T(value), decay_T(f4.get()));
}
} // namespace

TEST(std_future, shared_future) {
  int i{};
  test_shared_future<int>(i);
  test_shared_future<int &>(i);
  test_shared_future<const int &>(i);
  test_shared_future<int (*)(int)>(fi);
  test_shared_future<int (&)(int)>(fi);
}

namespace {
template <typename T> void test_promise(T value) {
  promise<T> p0;
  promise<T> p1(std::move(p0));
  p0 = std::move(p1);
  swap(p0, p1);
  p1.set_value(value);
  typedef std::decay_t<T> decay_T; // avoid function references
  EXPECT_EQ(decay_T(value), decay_T(p1.get_future().get()));
}
} // namespace

TEST(std_future, promise) {
  int i{};
  test_promise<int>(i);
  test_promise<int &>(i);
  test_promise<const int &>(i);
  test_promise<int (*)(int)>(fi);
  test_promise<int (&)(int)>(fi);
}

namespace {
template <typename R, typename... Args, typename F>
void test_packaged_task(F &&f, Args... args) {
  (packaged_task<R(Args...)>(f));

  packaged_task<R(Args...)> t0;
  packaged_task<R(Args...)> t1(std::forward<F>(f));
  packaged_task<R(Args...)> t2(std::move(t0));
  t0 = std::move(t2);
  swap(t0, t1);
  EXPECT_TRUE(t0.valid());
  EXPECT_FALSE(t1.valid());
  EXPECT_FALSE(t2.valid());

  auto f0 = t0.get_future();
  EXPECT_TRUE(f0.valid());
  EXPECT_FALSE(f0.wait_for(std::chrono::seconds(0)) == future_status::ready);
  t0(args...);
  EXPECT_TRUE(t0.valid());
  // EXPECT_TRUE(f0.wait_for(std::chrono::seconds(0)) == future_status::ready);
  // EXPECT_EQ(1,f0.get());
  f0.get();

  t0.reset();
  EXPECT_TRUE(t0.valid());
  t0(std::forward<Args>(args)...);
  auto f1 = t0.get_future();
  EXPECT_TRUE(f1.wait_for(std::chrono::seconds(0)) == future_status::ready);
  // EXPECT_EQ(1,f1.get());
  f1.get();
}
} // namespace

TEST(std_future, packaged_task) {
  test_packaged_task<int, int>(fi, 0);
  test_packaged_task<int, int>(&fi, 0);
  test_packaged_task<int, int>(std::function<int(int)>(fi), 0);
  test_packaged_task<int, int>([](int x) { return x; }, 0);
  test_packaged_task<void, int>(fv, 0);
  test_packaged_task<void, int>(&fv, 0);
  test_packaged_task<void, int>(std::function<void(int)>(fv), 0);
  test_packaged_task<void, int>([](int) {}, 0);
  test_packaged_task<void, int>([](int) mutable {}, 0);
  // test_packaged_task<void, int>([f = future<void>()](int)mutable {}, 0);
  test_packaged_task<void, int>([f = shared_future<void>()](int) {}, 0);
  test_packaged_task<void, int>([f = shared_future<void>()](int) mutable {}, 0);
}

TEST(std_future, async) {
  auto ffi = async(fi, 1);
  EXPECT_EQ(1, ffi.get());
  auto ffv = async(fv, 1);
  auto flvv = async([]() {});
  auto flii = async([](int x) { return x; }, 1);
  EXPECT_EQ(1, flii.get());
  auto fliv = async([](int) {}, 1);

  auto fa = async(launch::async, fi, 1);
  this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(fa.wait_for(std::chrono::seconds(0)) ==
              future_status::ready); // this is unreliable
  EXPECT_EQ(1, fa.get());
  auto fd = async(launch::deferred, fi, 1);
  this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_FALSE(fd.wait_for(std::chrono::seconds(0)) == future_status::ready);
  EXPECT_EQ(1, fd.get());
  // auto fs = async(launch::sync, fi, 1);
  // EXPECT_TRUE(fs.wait_for(std::chrono::seconds(0)) == future_status::ready);
  // EXPECT_EQ(1,fs.get());
  // auto fe = async(launch::detached, fi, 1);
  // EXPECT_FALSE(fe.valid());
}

namespace {
int recurse(int count) {
  if (count <= 1)
    return count;
  auto t0 = async(recurse, count / 2);
  auto t1 = async(recurse, count - count / 2);
  return t0.get() + t1.get();
}
} // namespace

TEST(std_future, async_many) {
  int maxcount = 100; // TODO 1000
  auto res = recurse(maxcount);
  EXPECT_EQ(maxcount, res);
}

namespace {
future<int> recurse2(int count) {
  if (count <= 1) {
    promise<int> p;
    p.set_value(count);
    return p.get_future();
  }
  auto t0 = recurse2(count / 2).share();
  auto t1 = recurse2(count - count / 2).share();
  return async([=]() { return t0.get() + t1.get(); });
}
} // namespace

TEST(std_future, async_many2) {
  int maxcount = 100; // TODO 1000
  auto res = recurse2(maxcount);
  EXPECT_EQ(maxcount, res.get());
}

namespace {
future<int> recurse3(int count) {
  if (count <= 1) {
    promise<int> p;
    p.set_value(count);
    return p.get_future();
  }
  auto t0 = recurse2(count / 2);
  auto t1 = recurse2(count - count / 2);
  return async([t0 = std::move(t0), t1 = std::move(t1)]() mutable {
    return t0.get() + t1.get();
  });
}
} // namespace

TEST(std_future, async_many3) {
  int maxcount = 100; // TODO 1000
  auto res = recurse3(maxcount);
  EXPECT_EQ(maxcount, res.get());
}
