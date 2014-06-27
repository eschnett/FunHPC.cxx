#ifndef HPX_HH
#define HPX_HH

#include "cxx_invoke.hh"

// TODO: include only the headers that are actually needed
// #include <hpx/hpx.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/future.hpp>
#include <hpx/include/threads.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/version.hpp>

#include <boost/thread/lock_guard.hpp>
// #include <boost/thread/mutex.hpp>

#include <chrono>
#include <utility>
#include <type_traits>

namespace hpx {

struct thread_stats_t {
  std::ptrdiff_t threads_started;
  std::ptrdiff_t threads_stopped;
};
thread_stats_t get_thread_stats();

int thread_main(int argc, char **argv);
void thread_initialize();
void thread_finalize();
}

namespace rpc {

using ::boost::lock_guard;
// using ::boost::mutex;

using ::hpx::async;
using ::hpx::future;
using ::hpx::launch;
using ::hpx::make_ready_future;
using ::hpx::promise;
using ::hpx::shared_future;
using ::hpx::thread;

using mutex = ::hpx::lcos::local::spinlock;

template <typename T> inline bool future_is_ready(const future<T> &f) {
  return f.is_ready();
}
template <typename T> inline bool future_is_ready(const shared_future<T> &f) {
  return f.is_ready();
}

template <typename T, typename F>
inline auto future_then(future<T> &&f, F &&func)
    -> future<typename rpc::invoke_of<F, future<T> &&>::type> {
  return std::move(f).then(func);
}
template <typename T, typename F>
inline auto future_then(const shared_future<T> &f, F &&func)
    -> future<typename rpc::invoke_of<F, const shared_future<T> &>::type> {
  return shared_future<T>(f).then(func);
}

namespace this_thread {
using ::hpx::this_thread::yield;
using ::hpx::this_thread::get_id;

// get_worker_id
inline std::size_t get_worker_id() { return ::hpx::get_worker_thread_num(); }

// sleep_for
template <typename Rep, typename Period>
inline void sleep_for(std::chrono::duration<Rep, Period> const &p) {
  typedef boost::ratio<Period::num, Period::den> PeriodBoost;
  auto pBoost = boost::chrono::duration<Rep, PeriodBoost>(p.count());
  ::hpx::this_thread::sleep_for(pBoost);
}
}

using hpx::thread_stats_t;
using hpx::get_thread_stats;

using ::hpx::thread_main;
using ::hpx::thread_initialize;
using ::hpx::thread_finalize;
}

namespace std {

// Poison std:: functionality that is also provided by HPX
struct hpx_incomplete;
typedef hpx_incomplete async;
typedef hpx_incomplete future;
typedef hpx_incomplete lock_guard;
typedef hpx_incomplete mutex;
typedef hpx_incomplete promise;
typedef hpx_incomplete shared_future;
typedef hpx_incomplete this_thread;
typedef hpx_incomplete thread;
}

namespace boost {

// Poison boost:: functionality that is also provided by HPX
struct hpx_incomplete;
// typedef hpx_incomplete async;
typedef hpx_incomplete future;
// typedef hpx_incomplete lock_guard;
// typedef hpx_incomplete mutex;
// typedef hpx_incomplete promise;
// typedef hpx_incomplete shared_future;
// typedef hpx_incomplete this_thread;
// typedef hpx_incomplete thread;
}

#define HPX_HH_DONE
#else
#ifndef HPX_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // HPX_HH
