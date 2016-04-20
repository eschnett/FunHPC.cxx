#ifndef QTHREAD_MUTEX_HPP
#define QTHREAD_MUTEX_HPP

#include <cxx/cassert.hpp>

#include <qthread/qt_syscalls.h>
#include <qthread/qthread.hpp>

#include <utility>

namespace qthread {

// mutex ///////////////////////////////////////////////////////////////////////

class mutex {
  syncvar mem;

  bool is_locked() { return !mem.status(); }

public:
  mutex() noexcept {}
  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  ~mutex() {
    cxx_assert(!is_locked());
    if (is_locked())
      unlock();
  }
  mutex &operator=(const mutex &) = delete;
  mutex &operator=(mutex &&) = delete;
  void lock() { mem.readFE(); }
  // bool try_lock();
  void unlock() {
    cxx_assert(is_locked());
    mem.fill();
  }
};

// unique_lock /////////////////////////////////////////////////////////////////

template <typename M> class unique_lock {
  M *mtx;
  bool owned;

public:
  typedef M mutex_type;
  unique_lock() noexcept : mtx(nullptr), owned(false) {}
  unique_lock(const unique_lock &) = delete;
  unique_lock(unique_lock &&other) noexcept : mtx(other.mtx),
                                              owned(other.owned) {
    other.mtx = nullptr;
    other.owned = false;
  }
  explicit unique_lock(M &m) : mtx(&m), owned(false) { lock(); }
  ~unique_lock() {
    if (owned)
      unlock();
  }
  unique_lock &operator=(const unique_lock &) = delete;
  unique_lock &operator=(unique_lock &&other) {
    if (owned)
      unlock();
    mtx = other.mtx;
    owned = other.owned;
    other.mtx = nullptr;
    other.owned = false;
    return *this;
  }
  M *mutex() const noexcept { return mtx; }
  bool owns_lock() const noexcept { return owned; }
  operator bool() const noexcept { return owned; }
  void lock() {
    cxx_assert(!owned);
    mtx->lock();
    owned = true;
  }
  // bool try_lock();
  void unlock() {
    cxx_assert(owned);
    mtx->unlock();
    owned = false;
  }
  void swap(unique_lock &other) noexcept {
    using namespace std;
    std::swap(mtx, other.mtx);
    std::swap(owned, other.owned);
  }
  M *release() noexcept {
    M *res = mtx;
    mtx = nullptr;
    owned = false;
    return res;
  }
};
template <typename M>
void swap(unique_lock<M> &lhs, unique_lock<M> &rhs) noexcept {
  lhs.swap(rhs);
}

// lock_guard //////////////////////////////////////////////////////////////////

template <typename M> class lock_guard {
  M &mtx;

public:
  typedef M mutex_type;
  explicit lock_guard(M &m) : mtx(m) { mtx.lock(); }
  lock_guard(const lock_guard &) = delete;
  lock_guard(lock_guard &&) = delete;
  ~lock_guard() { mtx.unlock(); }
  lock_guard &operator=(const lock_guard &) = delete;
  lock_guard &operator=(lock_guard &&) = delete;
};
}

#define QTHREAD_MUTEX_HPP_DONE
#endif // #ifndef QTHREAD_MUTEX_HPP
#ifndef QTHREAD_MUTEX_HPP_DONE
#error "Cyclic include dependency"
#endif
