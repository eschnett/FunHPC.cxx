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

// Integer division, no rounding
template <typename T> constexpr T div_exact(T x, T y) {
  assert(y != 0);
  T r = x / y;
  assert(r * y == x);
  return r;
}

// Integer modulo, no rounding
template <typename T> constexpr T mod_exact(T x, T y) {
  T r = x - div_exact(x, y) * y;
  assert(r == 0);
  return r;
}

// Integer align, no rounding
template <typename T> constexpr T align_exact(T x, T y) {
  assert(y != 0);
  T r = div_exact(x, y) * y;
  assert(r == x && mod_exact(r, y) == 0);
  return r;
}

// Integer division, rounding down
template <typename T> constexpr T div_floor(T x, T y) {
  assert(y > 0);
  T r = (x >= 0 ? x : x - y + 1) / y;
  assert(r * y <= x && (r + 1) * y > x);
  return r;
}

// Integer modulo, rounding down
template <typename T> constexpr T mod_floor(T x, T y) {
  T r = x - div_floor(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

// Integer align, rounding down
template <typename T> constexpr T align_floor(T x, T y) {
  assert(y > 0);
  T r = div_floor(x, y) * y;
  assert(r <= x && x - r < y && mod_floor(r, y) == 0);
  return r;
}

// Integer division, rounding up
template <typename T> constexpr T div_ceil(T x, T y) {
  assert(y > 0);
  T r = (x > 0 ? x + y - 1 : x) / y;
  assert(r * y >= x && (r - 1) * y < x);
  return r;
}

// Integer modulo, rounding up
template <typename T> constexpr T mod_ceil(T x, T y) {
  T r = x - div_ceil(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

// Integer align, rounding up
template <typename T> constexpr T align_ceil(T x, T y) {
  assert(y > 0);
  T r = div_ceil(x, y) * y;
  assert(r >= x && r - x < y && mod_ceil(r, y) == 0);
  return r;
}

// Integer power x^n
template <typename T> constexpr T ipow(T x, T n) {
  assert(n >= 0);
  T r = n & 1 ? x : 1;
  while (n >>= 1) {
    x *= x;
    if (n & 1)
      r *= x;
  }
  return r;
}

// Integer root root_n(x), rounded down
template <class T> constexpr T iroot(T n, T x) {
  assert(x >= 0 && n > 0);
  if (n == 1)
    return x;
  T b = 1;
  while (ipow(b, n) <= x)
    b <<= 1;
  T r = 0;
  while (b >>= 1)
    if (ipow(r + b, n) <= x)
      r += b;
  assert(ipow(r, n) <= x && ipow(r + 1, n) > x);
  return r;
}

// Integer log log_b(x), rounded down
template <class T> constexpr T ilog(T b, T x) {
  assert(b > 1 && x > 0);
  T n = 1;
  while (ipow(b, n) <= x)
    n <<= 1;
  T r = 0;
  while (n >>= 1)
    if (ipow(b, r + n) <= x)
      r += n;
  assert(ipow(b, r) <= x && ipow(b, r + 1) > x);
  return r;
}

////////////////////////////////////////////////////////////////////////////////

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

// Find first set (true) element. Return sizeof... if all elements are false.
template <bool...> struct ffs;
template <> struct ffs<> : std::integral_constant<size_t, 0> {};
template <bool... xs>
struct ffs<true, xs...> : std::integral_constant<size_t, 0> {};
template <bool... xs>
struct ffs<false, xs...>
    : std::integral_constant<size_t, ffs<xs...>::value + 1> {};
}

#define CXX_UTILS_HH_DONE
#else
#ifndef CXX_UTILS_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_UTILS_HH
