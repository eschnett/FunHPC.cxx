#ifndef QTHREAD_THREAD_HH
#define QTHREAD_THREAD_HH

#include "qthread_thread_fwd.hh"

#include "qthread_future.hh"
#include "qthread_mutex.hh"

#include <qthread/qthread.hpp>
#include <qthread/qt_syscalls.h>

#include "cxx_invoke.hh"
#include "cxx_tuple.hh"
#include "cxx_utils.hh"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace qthread {

// TODO: Introduce packaged_task; use this instead of function, and
// instead of turning thread into a template

struct thread_stats_t {
  std::ptrdiff_t threads_started;
  std::ptrdiff_t threads_stopped;
};
thread_stats_t get_thread_stats();

// TODO: Turn this into a template on the return type. Use this in
// async to create a future holding the return value.
class thread {

public:
  static std::atomic<std::ptrdiff_t> threads_started;
  static std::atomic<std::ptrdiff_t> threads_stopped;

private:
  struct thread_args {
    std::function<void()> func;
    promise<void> p;
    thread_args(const std::function<void()> &func) : func(func) {}
  };

  static aligned_t run_thread(void *args_);
  static future<void> start_thread(const std::function<void()> &func);

  future<void> handle;

public:
  typedef unsigned int id;

  thread() {}
  thread(thread &&other) : thread() { swap(other); }

  template <typename F, typename... As>
  explicit thread(const F &func, As &&... args) {
    // std::function<void()> funcbnd =
    //   std::bind<void>(func, std::forward<As>(args)...);
    auto funcptr = std::make_shared<typename std::decay<F>::type>(func);
    auto argsptr =
        std::make_shared<std::tuple<typename std::decay<As>::type...> >(
            std::forward<As>(args)...);
    std::function<void()> funcbnd =
        [funcptr, argsptr]() { cxx::tuple_apply(*funcptr, *argsptr); };
    handle = start_thread(funcbnd);
  }

  thread(const thread &) = delete;

  ~thread() {
    if (joinable())
      std::terminate();
  }

  thread &operator=(thread &&other) {
    if (joinable())
      std::terminate();
    swap(other);
    return *this;
  }

  bool joinable() const { return handle.valid(); }

  static unsigned int hardware_concurrency() { return qthread_num_workers(); }

  void join() {
    RPC_ASSERT(joinable());
    handle.wait();
    assert(!joinable());
  }

  void detach() { handle = future<void>(); }

  void swap(thread &other) { std::swap(handle, other.handle); }
};

namespace this_thread {

inline void yield() {
  qthread_yield();
  // qthread_yield_near();
}

inline thread::id get_id() { return qthread_id(); }

inline thread::id get_worker_id() { return qthread_worker(NULL); }

template <typename Rep, typename Period>
void sleep_for(const std::chrono::duration<Rep, Period> &duration) {
  const auto usecs = std::chrono::microseconds(duration).count();
  timeval timeout;
  timeout.tv_sec = usecs / 1000000;
  timeout.tv_usec = usecs % 1000000;
  qt_select(0, nullptr, nullptr, nullptr, &timeout);
}
}

template <typename F, typename... As>
auto async(launch policy, const F &func, As &&... args)
    -> typename std::enable_if<
          !std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  bool is_deferred = policy == launch::deferred;
  bool is_sync = (policy & launch::sync) == launch::sync;
  auto funcptr = std::make_shared<typename std::decay<F>::type>(func);
  auto argsptr =
      std::make_shared<std::tuple<typename std::decay<As>::type...> >(
          std::forward<As>(args)...);
  typedef typename cxx::invoke_of<F, As...>::type R;
  if (is_deferred) {
    auto funcbnd = [funcptr, argsptr]() -> R {
      return cxx::tuple_apply(*funcptr, *argsptr);
    };
    auto d = deferred<R>(funcbnd);
    auto f = d.get_future();
    return f;
  } else if (is_sync) {
    return make_ready_future(cxx::tuple_apply(*funcptr, *argsptr));
  } else {
    auto p = new promise<R>();
    auto f = p->get_future();
    auto funcbnd = [funcptr, argsptr, p]() {
      p->set_value(cxx::tuple_apply(*funcptr, *argsptr));
      delete p;
    };
    thread(funcbnd).detach();
    return f;
  }
}

template <typename F, typename... As>
auto async(launch policy, const F &func, As &&... args)
    -> typename std::enable_if<
          std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  bool is_deferred = policy == launch::deferred;
  bool is_sync = (policy & launch::sync) == launch::sync;
  auto funcptr = std::make_shared<typename std::decay<F>::type>(func);
  auto argsptr =
      std::make_shared<std::tuple<typename std::decay<As>::type...> >(
          std::forward<As>(args)...);
  if (is_deferred) {
    auto funcbnd =
        [funcptr, argsptr]() { cxx::tuple_apply(*funcptr, *argsptr); };
    auto d = deferred<void>(funcbnd);
    auto f = d.get_future();
    return f;
  } else if (is_sync) {
    cxx::tuple_apply(*funcptr, *argsptr);
    return make_ready_future();
  } else {
    auto p = std::make_shared<promise<void> >();
    auto f = p->get_future();
    auto funcbnd = [funcptr, argsptr, p]() {
      cxx::tuple_apply(*funcptr, *argsptr);
      p->set_value();
    };
    thread(funcbnd).detach();
    return f;
  }
}

template <typename F, typename... As>
auto async(const F &func, As &&... args) ->
    // typename std::enable_if<!std::is_same<F, launch>::value,
    //                         future<typename cxx::invoke_of<F, As...>::type>
    //                         >::type
    future<typename cxx::invoke_of<F, As...>::type> {
  return async(launch::async | launch::deferred, func,
               std::forward<As>(args)...);
}

#if 0
  // Add pointer to promise to shared state?
  void when_any();              // TODO
  
  
  
  template<typename Iter>
  typename std::enable_if<
    !std::is_void<typename detail::future_traits<typename std::iterator_traits<Iter>::value_type>::value_type>::value,
    future<std::vector<typename detail::future_traits<typename std::iterator_traits<Iter>::value_type>::value_type> >
    >::type
  when_all(const Iter& begin, const Iter& end)
  {
    return async([begin, end]() {
        std::vector<typename detail::future_traits<typename std::iterator_traits<Iter>::value_type>::value_type> res;
        for (auto it = begin; it != end; ++it) {
          res.push_back(it->get());
        }
        return res;
      });
  }
  
  template<typename Iter>
  typename std::enable_if<
    std::is_void<typename detail::future_traits<typename std::iterator_traits<Iter>::value_type>::value_type>::value,
    future<void>
    >::type
  when_all(const Iter& begin, const Iter& end)
  {
    return async([begin, end]() {
        for (auto it = begin; it != end; ++it) {
          it->get();
        }
      });
  }
  
  namespace detail {
    template<typename... As> struct all;
    template<> struct all<> { static constexpr bool value = true; };
    template<typename A0, typename... As>
    struct all<A0, As...>
    {
      static constexpr bool value = A0::value && all<As...>::value;
    };
    
    template<typename... As> struct any;
    template<> struct any<> { static constexpr bool value = false; };
    template<typename A0, typename... As>
    struct any<A0, As...>
    {
      static constexpr bool value = A0::value || any<As...>::value;
    };
  }
  
  template<typename... As>
  struct when_all_impl {
    typedef std::tuple<typename detail::future_traits<As>::value_type...>
    result_type;
    
    template<typename Tuple>
    static result_type apply_get_impl(Tuple&& t)
    {
      return std::forward<Tuple>(t);
    }
    
    template<typename Tuple, typename B0, typename... Bs>
    static result_type apply_get_impl(Tuple&& t, B0&& arg0, Bs&&... args)
    {
      return apply_get_impl(std::tuple_cat(std::forward<Tuple>(t),
                                           std::make_tuple
                                           (std::forward<B0>(arg0).get())),
                            std::forward<Bs>(args)...);
    }
    
    static result_type apply_get(As&&... args)
    {
      return apply_get_impl(std::make_tuple(), std::forward<As>(args)...);
    }
  };
  
  template<typename... As>
  typename std::enable_if<
    detail::all<std::integral_constant<bool, !std::is_void<typename detail::future_traits<As>::value_type>::value>...>::value,
    future<std::tuple<typename detail::future_traits<As>::value_type...> >
    >::type
  when_all(As&&... args)
  {
    return async(when_all_impl<As...>::apply_get, std::forward<As>(args)...);
  }
  
  template<typename... As>
  struct when_all_void_impl {
    static void apply_get_impl()
    {
    }
    
    template<typename B0, typename... Bs>
    static void apply_get_impl(B0&& arg0, Bs&&... args)
    {
      std::forward<B0>(arg0).get();
      apply_get_impl(std::forward<Bs>(args)...);
    }
    
    static void apply_get(As&&... args)
    {
      return apply_get_impl(std::forward<As>(args)...);
    }
  };
  
  template<typename... As>
  typename std::enable_if<
    (sizeof...(As)>0) && detail::all<std::integral_constant<bool, std::is_void<typename detail::future_traits<As>::value_type>::value>...>::value,
    future<void>
    >::type
  when_all(As&&... args)
  {
    return async(when_all_void_impl<As...>::apply_get, std::forward<As>(args)...);
  }

#if 0
  namespace detail {
    template<typename... As>
    void when_all_void_impl(As&&... args);
    template<typename A0, typename... As>
    void when_all_void_impl(A0&& arg0, As&&... args)
    {
      std::forward<A0>(arg0).get();
      when_all_void_impl(std::forward<As>(args)...);
    }
    template<>
    void when_all_void_impl<>()
    {
    }
  }
  
  template<typename... As>
  typename std::enable_if<
    (sizeof...(As)>0) && detail::all<std::integral_constant<bool, std::is_void<typename detail::future_traits<As>::value_type>::value>...>::value,
    future<void>
    >::type
  when_all(As&&... args)
  {
    return detail::when_all_impl(std::forward<As>(args)...);
  }
#endif

#endif

int thread_main(int argc, char **argv);
void thread_initialize();
void thread_finalize();
}

#define QTHREAD_THREAD_HH_DONE
#else
#ifndef QTHREAD_THREAD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // QTHREAD_THREAD_HH
