#ifndef QTHREAD_MUTEX_HH
#define QTHREAD_MUTEX_HH

#include <qthread.h>



namespace qthread {
  
  class mutex {
    aligned_t mem;
  public:
    mutex() {}
    mutex(const mutex&) = delete;
    mutex(mutex&&) = delete;
    mutex& operator=(const mutex&) = delete;
    mutex& operator=(mutex&&) = delete;
    void lock() { qthread_lock(&mem); }
    // TODO: implement this
    bool try_lock();
    void unlock() { qthread_unlock(&mem); }
  };
  
  template<typename M>
  class lock_guard {
    M& mtx;
  public:
    explicit lock_guard(M& m): mtx(m) { mtx.lock(); }
    ~lock_guard() { mtx.unlock(); }
  };
  
}

#define QTHREAD_MUTEX_HH_DONE
#else
#  ifndef QTHREAD_MUTEX_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_MUTEX_HH
