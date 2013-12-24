#ifndef QTHREAD_HH
#define QTHREAD_HH

#include "qthread_future.hh"
#include "qthread_mutex.hh"
#include "qthread_thread.hh"

namespace rpc {
  
  using ::qthread::async;
  using ::qthread::future;
  using ::qthread::launch;
  using ::qthread::lock_guard;
  using ::qthread::mutex;
  using ::qthread::promise;
  using ::qthread::shared_future;
  namespace this_thread {
    using ::qthread::this_thread::sleep_for;
    using ::qthread::this_thread::yield;
  }
  using ::qthread::thread;
  
  using ::qthread::thread_main;
  using ::qthread::thread_initialize;
  using ::qthread::thread_finalize;
  using ::qthread::thread_finalize2;
  
}

namespace std {
  
  // Poison std:: functionality that is also provided by qthread
  struct qthread_incomplete;
  typedef qthread_incomplete async;
  typedef qthread_incomplete future;
  typedef qthread_incomplete lock_guard;
  typedef qthread_incomplete mutex;
  typedef qthread_incomplete promise;
  typedef qthread_incomplete shared_future;
  typedef qthread_incomplete this_thread;
  typedef qthread_incomplete thread;
  
}

#define QTHREAD_HH_DONE
#else
#  ifndef QTHREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_HH
