#ifndef HPX_HH
#define HPX_HH

// TODO: include only the headers that are actually needed
// #include <hpx/hpx.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/future.hpp>
#include <hpx/include/threads.hpp>

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include <chrono>
#include <utility>
#include <type_traits>

namespace hpx {
  int thread_main(int argc, char** argv);
  void thread_initialize();
  void thread_finalize();
  void thread_finalize2();
}

namespace rpc {
  
  using ::boost::lock_guard;
  using ::boost::mutex;
  
  using ::hpx::async;
  using ::hpx::launch;
  using ::hpx::make_ready_future;
  using ::hpx::promise;
  using ::hpx::thread;
  
#if 0
  // use unique_future and shared_future
  
  // future
  template<typename T> using future = ::hpx::unique_future<T>;
  
  using ::hpx::shared_future;
  
  // async
  template<typename T> class client;
  namespace detail {
    template<typename T>
    struct is_client { static constexpr bool value = false; };
    template<>
    template<typename T>
    struct is_client<client<T>> { static constexpr bool value = true; };
  }
  template<typename A0, typename... As>
  auto async(A0 arg0, As... args) ->
    typename std::enable_if<
      !std::is_same<A0, int>::value && !detail::is_client<A0>::value,
      decltype(::hpx::async(std::forward<A0>(arg0),
                            std::forward<As>(args)...).unique())>::type
  {
    return ::hpx::async(std::forward<A0>(arg0),
                        std::forward<As>(args)...).unique();
  }
  
#else
  // use future and shared_future
  
  using ::hpx::future;
  using ::hpx::shared_future;
  
#endif
  
  namespace this_thread {
    using ::hpx::this_thread::yield;
    
    // sleep_for
    template<typename Rep, typename Period>
    inline void sleep_for(std::chrono::duration<Rep, Period> const& p)
    {
      typedef boost::ratio<Period::num, Period::den> PeriodBoost;
      auto pBoost = boost::chrono::duration<Rep, PeriodBoost>(p.count());
      ::hpx::this_thread::sleep_for(pBoost);
    }
  }
  
  using ::hpx::thread_main;
  using ::hpx::thread_initialize;
  using ::hpx::thread_finalize;
  using ::hpx::thread_finalize2;
  
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

#define HPX_HH_DONE
#else
#  ifndef HPX_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // HPX_HH
