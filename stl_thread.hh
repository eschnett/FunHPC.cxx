#ifndef STL_THREAD_HH
#define STL_THREAD_HH

#include "cxx_invoke.hh"

#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace rpc {

using ::std::async;
using ::std::decay;
using ::std::future;
using ::std::launch;
using ::std::lock_guard;
using ::std::mutex;
using ::std::promise;
using ::std::shared_future;
namespace this_thread {
using ::std::this_thread::sleep_for;
using ::std::this_thread::yield;
}
using ::std::thread;

template <typename T>
future<typename std::decay<T>::type> make_ready_future(T &&obj) {
  promise<typename std::decay<T>::type> p;
  p.set_value(std::forward<T>(obj));
  return p.get_future();
}
inline future<void> make_ready_future() {
  promise<void> p;
  p.set_value();
  return p.get_future();
}

template <typename T> inline bool future_is_ready(const future<T> &f) {
  return false; // punt
}

template <typename T> inline bool future_is_ready(const shared_future<T> &f) {
  return false; // punt
}

template <typename T, typename F>
inline auto future_then(future<T> &&f, F &&func)
    -> future<typename rpc::invoke_of<F, future<T> &&>::type> {
  auto f0 = new future<T>(std::move(f));
  return async([=]() {
    std::unique_ptr<future<T> > f(f0);
    f->wait();
    return rpc::invoke(func, std::move(*f));
  });
}

template <typename T, typename F>
inline auto future_then(const shared_future<T> &f, F &&func)
    -> future<typename rpc::invoke_of<F, const shared_future<T> &>::type> {
  return async([=]() {
    f.wait();
    return rpc::invoke(func, f);
  });
}

namespace this_thread {
inline int get_worker_id() { return 0; }
}

struct thread_stats_t {
  std::ptrdiff_t threads_started;
  std::ptrdiff_t threads_stopped;
};
inline thread_stats_t get_thread_stats() {
  return { 0, 0 };
}

int real_main(int argc, char **argv);
inline int thread_main(int argc, char **argv) { return real_main(argc, argv); }
inline void thread_initialize() {}
inline void thread_finalize() {}
}

#define STL_THREAD_HH_DONE
#else
#ifndef STL_THREAD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // STL_THREAD_HH
