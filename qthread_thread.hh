#ifndef QTHREAD_THREAD_HH
#define QTHREAD_THREAD_HH

#include <qthread.h>
#include <qthread/qt_syscalls.h>

#include "qthread_future.hh"
#include "qthread_mutex.hh"

#include <atomic>
#include <chrono>
#include <functional>
#include <vector>



namespace qthread {
  
  class detached_threads;
  extern detached_threads* detached;
  
  
  
  class thread_manager {
    
    // The function to call
    std::function<void()> func;
    
    static aligned_t wrapper(void* mgr)
    {
      ((thread_manager*)mgr)->func();
      return 1;
    }
    
    // We initialize ret to 0, and set it to 1 once the thread has
    // finished
    aligned_t m_done;
    
  public:
    
    thread_manager(const std::function<void()>& func):
      func(func), m_done(0)
    {
      int ierr = qthread_fork(wrapper, this, &m_done);
      assert(!ierr);
    }
    
    ~thread_manager()
    {
      aligned_t tmp;
      qthread_readFF(&tmp, &m_done);
      assert(done());
    }
    
    bool done() const { return (volatile aligned_t&)m_done; }
  };
  
  
  
  class thread {
    
    thread_manager* mgr;
    
  public:
    
    typedef thread_manager* native_handle_type;
    
    thread(): mgr(nullptr) {}
    thread(thread&& other): thread() { swap(other); }
    
    template<typename F, typename... As>
    explicit thread(F&& f, As&&... args):
      mgr(new thread_manager(std::bind(f, args...)))
    {
    }
    
    thread(const thread&) = delete;
    
    ~thread() {
      if (joinable()) std::terminate();
    }
    
    thread& operator=(thread&& other)
    {
      if (mgr) std::terminate();
      swap(other);
      return *this;
    }
    
    bool joinable() const { return mgr; }
    
    static unsigned int hardware_concurrency() { return qthread_num_workers(); }
    
    void join() { delete mgr; mgr = nullptr; }
    
    void detach();
    
    void swap(thread& other) { std::swap(mgr, other.mgr); }
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
  auto async(F&& f, As&&... args) ->
    typename std::enable_if<
      (!std::is_same<F, launch>::value &&
       !std::is_void<typename std::result_of<F(As...)>::type>::value),
      future<typename std::result_of<F(As...)>::type> >::type
  {
    typedef typename std::result_of<F(As...)>::type R;
    auto prm = std::make_shared<promise<R>>();
    // gcc does not handle lambda expressions with parameter packs
    auto fbnd = std::bind(f, args...);
    auto func = [prm, fbnd]() mutable {
      prm->set_value(fbnd());
    };
    thread(func).detach();
    return prm->get_future();
  }
  
  template<typename F, typename... As>
  auto async(F&& f, As&&... args) ->
    typename std::enable_if<
      (!std::is_same<F, launch>::value &&
       std::is_void<typename std::result_of<F(As...)>::type>::value),
      future<typename std::result_of<F(As...)>::type> >::type
  {
    auto prm = std::make_shared<promise<void>>();
    auto fbnd = std::bind(f, args...);
    auto func = [prm, fbnd]() {
      fbnd();
      prm->set_value();
    };
    thread(func).detach();
    return prm->get_future();
  }
  
  template<typename F, typename... As>
  auto async(launch l, F&& f, As&&... args) ->
    decltype(async(std::forward<F>(f), std::forward<As>(args)...))
  {
    return async(std::forward<F>(f), std::forward<As>(args)...);
  }  
  
  
  
  class detached_threads {
    
    // TODD: Use a lock-free data structure such as qlfqueue or
    // similar?
    mutex mtx;
    std::vector<thread_manager*> detached;
    std::vector<thread_manager*> incoming;
    
    thread cleanup_thread;
    std::atomic<bool> signal_stop;
    
  public:
    
    ~detached_threads();
    
    void start_cleanup();
    void stop_cleanup();
    
    void add(thread_manager* mgr)
    {
      lock_guard<mutex> g(mtx);
      incoming.push_back(mgr);
    }
    
  private:
    
    // Periodically clean up detached threads
    void cleanup();
  };
  
  
  
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
