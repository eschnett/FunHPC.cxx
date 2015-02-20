#ifndef FUNHPC_RPTR_HPP
#define FUNHPC_RPTR_HPP

#include <funhpc/rexec.hpp>

#include <cereal/access.hpp>

#include <cassert>
#include <cstdlib>
#include <utility>

namespace funhpc {

template <typename T> class rptr {
  std::ptrdiff_t proc;
  std::uintptr_t iptr;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(proc, iptr); }

public:
  typedef T element_type;

  rptr() noexcept : proc(-1), iptr(0) {}
  rptr(T *ptr) noexcept : proc(ptr ? rank() : -1), iptr(std::uintptr_t(ptr)) {}
  rptr(const rptr &other) noexcept : proc(other.proc), iptr(other.iptr) {}
  rptr &operator=(const rptr &other) noexcept {
    proc = other.proc;
    iptr = other.iptr;
    return *this;
  }
  void swap(rptr &other) noexcept {
    using std::swap;
    swap(proc, other.proc);
    swap(iptr, other.iptr);
  }

  operator bool() const noexcept { return bool(iptr); }
  bool local() const {
    assert(bool(*this));
    return proc == rank();
  }
  std::ptrdiff_t get_proc() const {
    assert(bool(*this));
    return proc;
  }
  T *get_ptr() const {
    if (!bool(*this))
      return nullptr;
    assert(local());
    return (T *)iptr;
  }
  const T &operator*() const { return *get_ptr(); }
  T &operator*() { return *get_ptr(); }
  const T &operator[](std::ptrdiff_t off) const { return get_ptr()[off]; }
  T &operator[](std::ptrdiff_t off) { return get_ptr()[off]; }
  const T *operator->() const { return get_ptr(); }
  T *operator->() { return get_ptr(); }

  bool operator==(const rptr &other) const noexcept {
    return proc == other.proc && iptr == other.iptr;
  }
  bool operator!=(const rptr &other) const noexcept {
    return !(*this == other);
  }
  bool operator<=(const rptr &other) const noexcept {
    return proc < other.proc || (proc == other.proc && iptr <= other.iptr);
  }
  bool operator>=(const rptr &other) const noexcept { return other <= *this; }
  bool operator<(const rptr &other) const noexcept { return !(*this >= other); }
  bool operator>(const rptr &other) const noexcept { return !(*this <= other); }
};
template <typename T> void swap(rptr<T> &lhs, rptr<T> &rhs) { lhs.swap(rhs); }
}

#define FUNHPC_RPTR_HPP_DONE
#endif // #ifdef FUNHPC_RPTR_HPP
#ifndef FUNHPC_RPTR_HPP_DONE
#error "Cyclic include dependency"
#endif
