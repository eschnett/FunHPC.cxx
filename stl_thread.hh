#ifndef STL_THREAD_HH
#define STL_THREAD_HH

#include <future>
#include <mutex>
#include <thread>
#include <utility>

namespace rpc {
  
  using ::std::async;
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
  future<T> make_ready_future(T&& obj)
  {
    promise<T> p;
    p.set_value(std::forward<T>(obj));
    return p.get_future();
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
