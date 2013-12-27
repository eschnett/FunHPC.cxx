#ifndef QTHREAD_THREAD_HH
#define QTHREAD_THREAD_HH

#include <qthread/qthread.hpp>
#include <qthread/qt_syscalls.h>

#include "qthread_future.hh"
#include "qthread_mutex.hh"

#include <chrono>
#include <functional>
#include <vector>
#include <utility>



namespace qthread {
  
  template<typename T>
  typename std::decay<T>::type decay_copy(T&& value)
  {
    return std::forward<T>(value);
  }
  
  
  
  class thread {
    
    struct thread_args {
      std::function<void()> func;
      promise<void> p;
      thread_args(const std::function<void()>& func): func(func) {}
    };
    
    static aligned_t run_thread(void* args_);
    static future<void> start_thread(const std::function<void()>& func);
    
    
    
    future<void> handle;
    
  public:
    
    thread() {}
    thread(thread&& other): thread() { swap(other); }
    
    template<typename F, typename... As>
    explicit thread(F&& f, As&&... args)
    {
      auto fb = std::bind(decay_copy<F>(std::forward<F>(f)),
                          decay_copy<As>(std::forward<As>(args))...);
      // auto fb = std::bind(std::forward<F>(f), std::forward<As>(args)...);
      handle = start_thread([fb]() { fb(); });
    }
    
    thread(const thread&) = delete;
    
    ~thread() {
      if (joinable()) std::terminate();
    }
    
    thread& operator=(thread&& other)
    {
      if (joinable()) std::terminate();
      swap(other);
      return *this;
    }
    
    bool joinable() const { return handle.valid(); }
    
    static unsigned int hardware_concurrency() { return qthread_num_workers(); }
    
    void join() { assert(joinable()); handle.wait(); assert(!joinable()); }
    
    void detach() { handle = future<void>(); }
    
    void swap(thread& other) { std::swap(handle, other.handle); }
  };
  
  
  
  namespace this_thread {
    
    inline void yield()
    {
      qthread_yield();
    }
    
    template<typename Rep, typename Period>
    void sleep_for(const std::chrono::duration<Rep,Period>& duration)
    {
      const auto usecs = std::chrono::microseconds(duration).count();
      timeval timeout;
      timeout.tv_sec = usecs / 1000000;
      timeout.tv_usec = usecs % 1000000;
      qt_select(0, nullptr, nullptr, nullptr, &timeout);
    }
    
  }
  
  
  
  enum class launch { async, deferred, sync };
  
  template<typename F, typename... As> 
  typename std::enable_if<
    !std::is_void<typename std::result_of<
                    typename std::decay<F>::type
                    (typename std::decay<As>::type...)>::type>::value,
    future<typename std::result_of<
             typename std::decay<F>::type
             (typename std::decay<As>::type...)>::type>
    >::type
  async(launch policy, F&& func, As&&... args)
  {
    typedef typename std::result_of<
      typename std::decay<F>::type(typename std::decay<As>::type...)>::type R;
    auto p = new promise<R>();
    auto f = p->get_future();
    // gcc does not handle lambda expressions with parameter packs
    auto func0 = std::bind(decay_copy<F>(std::forward<F>(func)),
                           decay_copy<As>(std::forward<As>(args))...);
    thread([p, func0]() {
        p->set_value(func0());
        delete p;
      }).detach();
    return f;
  }
  
  template<typename F, typename... As> 
  typename std::enable_if<
    std::is_void<typename std::result_of<
                   typename std::decay<F>::type
                   (typename std::decay<As>::type...)>::type>::value,
    future<typename std::result_of<
             typename std::decay<F>::type
             (typename std::decay<As>::type...)>::type>
    >::type
  async(launch policy, F&& func, As&&... args)
  {
    auto p = new promise<void>();
    auto f = p->get_future();
    // gcc does not handle lambda expressions with parameter packs
    auto func0 = std::bind(decay_copy<F>(std::forward<F>(func)),
                           decay_copy<As>(std::forward<As>(args))...);
    thread([p, func0]() {
        func0();
        p->set_value();
        delete p;
      }).detach();
    return f;
  }
  
  template<typename F, typename... As> 
  future<typename std::result_of<
           typename std::decay<F>::type
           (typename std::decay<As>::type...)>::type>
  async(F&& func, As&&... args)
  {
    return async(launch::async,
                 std::forward<F>(func), std::forward<As>(args)...);
  }
  
  
  
  int thread_main(int argc, char** argv);
  void thread_initialize();
  void thread_finalize();
  void thread_finalize2();
  
}

#define QTHREAD_THREAD_HH_DONE
#else
#  ifndef QTHREAD_THREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_THREAD_HH
