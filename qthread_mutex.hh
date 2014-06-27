#ifndef QTHREAD_MUTEX_HH
#define QTHREAD_MUTEX_HH

#include <qthread/qthread.hpp>

namespace qthread {

class mutex {
  syncvar mem;

public:
  mutex() {}
  ~mutex() { RPC_ASSERT(mem.status()); }
  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  mutex &operator=(const mutex &) = delete;
  mutex &operator=(mutex &&) = delete;
  void lock() { mem.readFE(); }
  // TODO: implement this
  bool try_lock();
  void unlock() {
    RPC_ASSERT(!mem.status());
    mem.fill();
  }
};

template <typename M> class lock_guard {
  M &mtx;

public:
  explicit lock_guard(M &m) : mtx(m) { mtx.lock(); }
  ~lock_guard() { mtx.unlock(); }
};
}

#define QTHREAD_MUTEX_HH_DONE
#else
#ifndef QTHREAD_MUTEX_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // QTHREAD_MUTEX_HH
