#ifndef CXX_CSTDLIB_HPP
#define CXX_CSTDLIB_HPP

#include <cassert>
#include <cstdlib>
#include <utility>

namespace cxx {

template <typename T> struct div_t {
  T quot;
  T rem;
};

template <typename T, typename U,
          typename R = decltype(std::declval<T>() / std::declval<U>())>
constexpr div_t<R> div_floor(const T x, const U y) {
  // x == q*y+r
  R q = x / y;
  R r = x % y;
  if (r < 0) {
    --q;
    r += y;
  }
  assert(r >= 0 && r < std::abs(y));
  assert(x == q * y + r);
  return {q, r};
}

template <typename T, typename U,
          typename R = decltype(std::declval<T>() / std::declval<U>())>
constexpr div_t<R> div_ceil(const T x, const U y) {
  // x == q*y+r
  R q = x / y;
  R r = x % y;
  if (r > 0) {
    ++q;
    r -= y;
  }
  assert(r <= 0 && r > -std::abs(y));
  assert(x == q * y + r);
  return {q, r};
}

template <typename T, typename U,
          typename R = decltype(std::declval<T>() / std::declval<U>())>
constexpr div_t<R> div_exact(const T x, const U y) {
  // x == q*y+r
  R q = x / y;
  R r = x % y;
  assert(r == 0);
  assert(x == q * y + r);
  return {q, r};
}
}

#define CXX_CSTDLIB_HPP_DONE
#endif // #ifdef CXX_CSTDLIB_HPP
#ifndef CXX_CSTDLIB_HPP_DONE
#error "Cyclic include dependency"
#endif
