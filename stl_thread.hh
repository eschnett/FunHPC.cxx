#ifndef STL_THREAD_HH
#define STL_THREAD_HH

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
  
  template<typename T>
  future<typename std::decay<T>::type> make_future(T&& obj)
  {
    promise<typename std::decay<T>::type> p;
    p.set_value(std::forward<T>(obj));
    return p.get_future();
  }
  inline future<void> make_future()
  {
    promise<void> p;
    p.set_value();
    return p.get_future();
  }
  
  template<typename T>
  shared_future<typename std::decay<T>::type> make_shared_future(T&& obj)
  {
    return make_future(std::forward<T>(obj)).share();
  }
  inline shared_future<void> make_shared_future()
  {
    return make_future().share();
  }
  
  namespace this_thread {
    inline int get_worker_id() { return 0; }
  }
  
  int real_main(int argc, char** argv);
  inline int thread_main(int argc, char** argv)
  {
    return real_main(argc, argv);
  }
  inline void thread_initialize() {}
  inline void thread_finalize() {}
  inline void thread_finalize2() {}
  
}

#define STL_THREAD_HH_DONE
#else
#  ifndef STL_THREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // STL_THREAD_HH
