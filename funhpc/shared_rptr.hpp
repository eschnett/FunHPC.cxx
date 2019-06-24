#ifndef FUNHPC_SHARED_RPTR_HPP
#define FUNHPC_SHARED_RPTR_HPP

#include <cxx/cassert.hpp>
#include <funhpc/rptr.hpp>
#include <qthread/future.hpp>

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <utility>

namespace funhpc {

// manager /////////////////////////////////////////////////////////////////////

namespace detail {
// TODO: make this abstract, i.e. independent of the type T
template <typename T> class manager {
  std::shared_ptr<T> obj;
  rptr<T> robj;
  mutable std::atomic<std::ptrdiff_t> refcount;
  rptr<manager> owner;

public:
  void incref() { ++refcount; }
  void decref() {
    if (--refcount == 0)
      delete this;
  }

  template <typename Archive> void save(Archive &ar) const {
    ar(robj);
    if (bool(robj)) {
      rptr<manager> origin(const_cast<manager *>(this));
      rptr<manager> owner1 = owner ? owner : origin;
      ++refcount;
      ar(owner1, origin);
    }
  }
  template <typename Archive> manager(Archive &ar) : manager() {
    ar(robj);
    if (bool(robj)) {
      rptr<manager> origin;
      ar(owner, origin);
      if (owner.get_proc() == rank()) {
        // The object is local: create a shortcut
        obj = owner->obj;
        owner = nullptr;
        rexec(origin.get_proc(), decref1, origin);
      } else {
        // Transfer refcount from origin to owner
        if (owner.get_proc() != origin.get_proc()) {
          // We must not destruct ourselves until the owner has
          // received the refcount. We temporarily increase our
          // refcount to prevent this.
          ++refcount;
          rptr<manager> self(this);
          rexec(owner.get_proc(), incref_then_decref2, owner, origin, self);
        }
      }
    }
    cxx_assert(invariant());
  }

private:
  static void incref1(rptr<manager> mgr) { mgr->incref(); }
  static void decref1(rptr<manager> mgr) { mgr->decref(); }
  static void incref_then_decref2(rptr<manager> mgr, rptr<manager> other1,
                                  rptr<manager> other2) {
    mgr->incref();
    rexec(other1.get_proc(), decref1, other1);
    rexec(other2.get_proc(), decref1, other2);
  }

public:
  manager() : obj(nullptr), robj(nullptr), refcount(1), owner(nullptr) {
    // This routine is only called for deserialization, and does not
    // need to return an object that is in a consistent state
    // cxx_assert(invariant());
  }
  manager(const std::shared_ptr<T> &obj)
      : obj(obj), robj(obj.get()), refcount(1), owner(nullptr) {
    cxx_assert(bool(obj));
    cxx_assert(invariant());
  }
  manager(std::shared_ptr<T> &&obj)
      : obj(std::move(obj)), robj(this->obj.get()), refcount(1),
        owner(nullptr) {
    cxx_assert(bool(this->obj));
    cxx_assert(invariant());
  }
  manager(manager &&other) = delete;
  manager(const manager &other) = delete;
  manager &operator=(manager &&other) = delete;
  manager &operator=(const manager &other) = delete;
  ~manager() {
    cxx_assert(refcount == 0);
    if (bool(owner))
      rexec(owner.get_proc(), decref1, owner);
  }

  bool local() const { return !bool(owner); }
  std::ptrdiff_t get_proc() const { return robj.get_proc(); }
  const std::shared_ptr<T> &get_shared_ptr() const {
    cxx_assert(local());
    return obj;
  }
  rptr<T> get_rptr() const { return robj; }

  bool invariant() const noexcept {
    if (local())
      return bool(obj) && robj.get_proc() == rank() &&
             robj.get_ptr() == obj.get() && !bool(owner) && refcount >= 1;
    return !bool(obj) && bool(robj) && robj.get_proc() != rank() &&
           bool(owner) && owner.get_proc() == robj.get_proc() && refcount >= 1;
  }
};
} // namespace detail

// shared_rptr /////////////////////////////////////////////////////////////////

template <typename T> class shared_rptr {
  detail::manager<T> *mgr;

  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const { mgr->save(ar); }
  template <typename Archive> void load(Archive &ar) {
    mgr = new detail::manager<T>(ar);
  }

public:
  typedef T element_type;

  shared_rptr() : mgr(nullptr) {}

  shared_rptr(const std::shared_ptr<T> &ptr) : shared_rptr() {
    if (bool(ptr))
      mgr = new detail::manager<T>(ptr);
  }
  shared_rptr(std::shared_ptr<T> &&ptr) : shared_rptr() {
    if (bool(ptr))
      mgr = new detail::manager<T>(std::move(ptr));
  }

  shared_rptr(const shared_rptr &other) : shared_rptr() {
    if (bool(other.mgr)) {
      other.mgr->incref();
      mgr = other.mgr;
    }
  }
  shared_rptr(shared_rptr &&other) : shared_rptr() { swap(other); }
  shared_rptr &operator=(const shared_rptr &other) {
    if (&other != this) {
      reset();
      if (bool(other.mgr)) {
        other.mgr->incref();
        mgr = other.mgr;
      }
    }
    return *this;
  }
  shared_rptr &operator=(shared_rptr &&other) {
    reset();
    swap(other);
    return *this;
  }
  void swap(shared_rptr &other) {
    using std::swap;
    swap(mgr, other.mgr);
  }
  void reset() {
    if (bool(mgr)) {
      mgr->decref();
      mgr = nullptr;
    }
  }
  ~shared_rptr() { reset(); }

  operator bool() const noexcept { return bool(mgr); }
  bool local() const {
    cxx_assert(bool(*this));
    return mgr->local();
  }
  std::ptrdiff_t get_proc() const {
    cxx_assert(bool(*this));
    return mgr->get_proc();
  }

  const std::shared_ptr<T> &get_shared_ptr() const {
    static const std::shared_ptr<T> null;
    if (!bool(*this))
      return null;
    cxx_assert(local());
    return mgr->get_shared_ptr();
  }
  const T &operator*() const {
    cxx_assert(bool(*this) && local());
    return *get_shared_ptr();
  }
  T &operator*() {
    cxx_assert(bool(*this) && local());
    return *get_shared_ptr();
  }
  const std::shared_ptr<T> &operator->() const { return get_shared_ptr(); }

  bool operator==(const shared_rptr &other) const {
    if (bool(*this) != bool(other))
      return false;
    if (!bool(*this))
      return true;
    return mgr->get_rptr() == other.mgr->get_rptr();
  }
  bool operator!=(const shared_rptr &other) const { return !(*this == other); }
  bool operator<=(const shared_rptr &other) const {
    if (!bool(*this))
      return true;
    if (bool(other))
      return false;
    return mgr->get_rptr() <= other.mgr->get_rptr();
  }
};
template <typename T> void swap(shared_rptr<T> &lhs, shared_rptr<T> &rhs) {
  lhs.swap(rhs);
}

// make_shared_rptr ////////////////////////////////////////////////////////////

template <typename T, typename... Args>
shared_rptr<T> make_shared_rptr(Args &&... args) {
  return shared_rptr<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

// make_local_shared_ptr ///////////////////////////////////////////////////////

namespace detail {
template <typename T>
std::shared_ptr<T> shared_rptr_get_shared_ptr(const shared_rptr<T> &rptr) {
  return rptr.get_shared_ptr();
}
} // namespace detail

template <typename T>
qthread::future<std::shared_ptr<T>>
make_local_shared_ptr(const shared_rptr<T> &rptr) {
  cxx_assert(bool(rptr));
  if (rptr.local())
    return qthread::make_ready_future(detail::shared_rptr_get_shared_ptr(rptr));
  return async(rlaunch::async | rlaunch::deferred, rptr.get_proc(),
               detail::shared_rptr_get_shared_ptr<T>, rptr);
}
} // namespace funhpc

#define FUNHPC_SHARED_RPTR_HPP_DONE
#endif // #ifdef FUNHPC_SHARED_RPTR_HPP
#ifndef FUNHPC_SHARED_RPTR_HPP_DONE
#error "Cyclic include dependency"
#endif
