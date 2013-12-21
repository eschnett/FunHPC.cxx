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
  
  using std::atomic;
  using std::bind;
  using std::enable_if;
  using std::function;
  using std::is_void;
  using std::result_of;
  using std::terminate;
  using std::vector;
  
  
  
  class detached_threads;
  extern detached_threads* detached;
  
  
  
  class thread_manager {
    
  public:
    
    typedef unsigned int id;
    
  private:
    
    // The function to call
    function<void()> func;
    
    // Promise for the thread id
    promise<id> p;
    
    static aligned_t wrapper(void* args)
    {
      thread_manager* mgr = (thread_manager*)args;
      mgr->p.set_value(qthread_id());
      mgr->func();
      return 1;
    }
    
    // We initialize ret to 0, and set it to 1 once the thread has
    // finished
    aligned_t m_done;
    
    shared_future<id> m_id;
    
  public:
    
    thread_manager(const function<void()>& func):
      func(func), p(), m_done(0), m_id(p.get_future())
    {
      int ierr = qthread_fork(wrapper, this, &m_done);
      assert(!ierr);
    }
    
    ~thread_manager()
    {
      join();
    }
    
    id get_id() const { return m_id.get(); }
    
    bool done() const { return (volatile aligned_t&)m_done; }
    
    void join()
    {
      aligned_t tmp;
      qthread_readFF(&tmp, &m_done);
    }
    
  };
  
  
  
  class thread {
    
    thread_manager* mgr;
    
  public:
    
    typedef thread_manager* native_handle_type;
    typedef thread_manager::id id;
    
    thread(): mgr(nullptr) {}
    thread(thread&& other): thread() { swap(other); }
    thread(const function<void()>& func): mgr(new thread_manager(func)) {}
    thread(const thread&) = delete;
    
    ~thread() {
      if (mgr) delete mgr;
    }
    
    thread& operator=(thread&& other)
    {
      if (mgr) terminate();
      swap(other);
      return *this;
    }
    
    bool joinable() const { return mgr; }
    
    id get_id() const { return mgr->get_id(); }
    
    static unsigned int hardware_concurrency() { return qthread_num_workers(); }
    
    void join() { mgr->join(); mgr = nullptr; }
    
    void detach();
    
    void swap(thread& other) { std::swap(mgr, other.mgr); }
  };
  
  
  
  namespace this_thread {
    
    inline thread::id get_id()
    {
      return qthread_id();
    }
    
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
  
  
  
  template<typename F, typename... As>
  auto async(F&& f, As&&... args) ->
    typename enable_if<!is_void<typename result_of<F(As...)>::type>::value,
                       future<typename result_of<F(As...)>::type> >::type
  {
    typedef typename result_of<F(As...)>::type R;
    auto prm = make_shared<promise<R>>();
    auto ftr = prm->get_future();
    // gcc does not handle lambda expressions with parameter packs
    auto fbnd = bind(f, args...);
    auto func = [prm, fbnd]() {
      prm->set_value(fbnd());
    };
    thread(func).detach();
    return ftr;
  }
  
  template<typename F, typename... As>
  auto async(F&& f, As&&... args) ->
    typename enable_if<is_void<typename result_of<F(As...)>::type>::value,
                       future<typename result_of<F(As...)>::type> >::type
  {
    typedef typename result_of<F(As...)>::type R;
    auto prm = make_shared<promise<R>>();
    auto ftr = prm->get_future();
    auto fbnd = bind(f, args...);
    auto func = [prm, fbnd]() {
      fbnd();
      prm->set_value();
    };
    thread(func).detach();
    return ftr;
  }
  
  
  
  class detached_threads {
    
    // TODD: Use a lock-free data structure such as qlfqueue or
    // similar?
    mutex mtx;
    vector<thread_manager*> detached;
    vector<thread_manager*> incoming;
    
    thread cleanup_thread;
    atomic<bool> signal_stop;
    
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
  
  
  
  void initialize();
  void finalize();
  
}

#define QTHREAD_THREAD_HH_DONE
#else
#  ifndef QTHREAD_THREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_THREAD_HH
