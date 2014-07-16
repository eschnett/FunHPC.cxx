#ifndef CXX_UTILS_HH
#define CXX_UTILS_HH

#include <cassert>
#include <iostream>
#include <memory>
#include <type_traits>

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef RPC_ASSERT
#define RPC_ASSERT(x) assert(x)
#endif

namespace cxx {

// special_decay, decay with special handling of reference_wrapper
template <typename T> struct special_decay {
  using type = typename std::decay<T>::type;
};
template <typename T> struct special_decay<std::reference_wrapper<T> > {
  using type = T &;
};

// decay_copy, taken from libc++ 3.4
template <typename T> inline typename std::decay<T>::type decay_copy(T &&t) {
  return std::forward<T>(t);
}

// Convert a type into a constant reference (const&)
template <typename T> struct const_ref {
  typedef const typename std::decay<T>::type &type;
};

// Assert wrapper
inline void rpc_assert(bool cond) {
  if (cond)
    return;
  std::cout << "ASSERTION FAILURE\n" << std::flush;
  std::cerr << "ASSERTION FAILURE\n" << std::flush;
  assert(0);
}

// make_ptr
template <typename T>
auto make_ptr(T &&value)
    -> decltype(new typename std::decay<T>::type(std::forward<T>(value))) {
  return new typename std::decay<T>::type(std::forward<T>(value));
}

// make_unique
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Identity function (different from decay_copy, this does not always
// copy the argument)
template <typename T> T identity(T &&x) { return std::forward<T>(x); }

// Identity type (mimicking a pointer)
// TODO: handle references, void
// TODO: add make_identity_type
// TODO: make this a monad
// TODO: should this be a container?
// TODO: use array<T,1> instead?
template <typename T> struct identity_type {
  typedef T type;
  T value;
  operator bool() const { return true; } // never null
  const T &operator*() const { return value; }
  T &operator*() { return value; }
  const T *operator->() const { return &value; }
  T *operator->() { return &value; }
};

// Boolean operations on template argument packs
template <bool...> struct all;
template <> struct all<> : std::true_type {};
template <bool... xs> struct all<true, xs...> : all<xs...> {};
template <bool... xs> struct all<false, xs...> : std::false_type {};

template <bool... xs>
struct any : std::integral_constant<bool, !all<!xs...>::value> {};

template <bool... xs>
struct none : std::integral_constant<bool, all<!xs...>::value> {};
}

#define CXX_UTILS_HH_DONE
#else
#ifndef CXX_UTILS_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // CXX_UTILS_HH
