#ifndef RPC_GLOBAL_PTR_FWD_HH
#define RPC_GLOBAL_PTR_FWD_HH

#include "rpc_future_fwd.hh"
#include "rpc_server.hh"

#include "cxx_utils.hh"

#include <cereal/archives/binary.hpp>

#include <cstdint>
#include <iostream>

namespace rpc {

template <typename T> class global_ptr;
template <typename T>
std::ostream &operator<<(std::ostream &os, const global_ptr<T> &ptr);

// A global pointer, represented as a combination of a local pointer
// and a process rank describing where the pointer is valid, i.e.
// where the pointee lives.
template <typename T> class global_ptr {

  int proc;
  std::uintptr_t uiptr;

public:
  typedef T element_type;
  typedef T value_type;

  bool invariant() const { return (proc == -1) == !uiptr; }

  // TODO: allow constructing from and converting to other
  // global_ptr, if the respective pointers can be converted (also
  // for global_shared_ptr and client)

  global_ptr(T *ptr = nullptr)
      : proc(ptr ? server->rank() : -1), uiptr(std::uintptr_t(ptr)) {
    RPC_ASSERT(invariant());
  }

  operator bool() const { return bool(uiptr); }
  int get_proc() const { return proc; }
  bool proc_is_ready() const { return true; }
  future<int> get_proc_future() const { return make_ready_future(proc); }
  bool is_local() const {
    // nullptr is always local
    return !*this || proc == server->rank();
  }

  bool operator==(const global_ptr &other) const {
    return proc == other.proc && uiptr == other.uiptr;
  }
  bool operator!=(const global_ptr &other) const { return !(*this == other); }

  T *get() const {
    RPC_ASSERT(is_local());
    return (T *)uiptr;
  }
  T &operator*() const { return *get(); }
  auto operator -> () const -> decltype(this -> get()) { return get(); }

  // future<shared_ptr<T> > make_local() const;

  std::ostream &output(std::ostream &os) const {
    return os << proc << ":" << (T *)uiptr;
  }

private:
  friend class cereal::access;
  template <class Archive> void serialize(Archive &ar) { ar(proc, uiptr); }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const global_ptr<T> &ptr) {
  return ptr.output(os);
}

template <typename T, typename... As>
global_ptr<T> make_global(const As &... args) {
  return new T(args...);
}
}

#define RPC_GLOBAL_PTR_FWD_HH_DONE
#else
#ifndef RPC_GLOBAL_PTR_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_GLOBAL_PTR_FWD_HH
