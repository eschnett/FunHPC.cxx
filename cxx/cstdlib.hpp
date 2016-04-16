#ifndef CXX_CSTDLIB_HPP
#define CXX_CSTDLIB_HPP

#include <cxx/cassert.hpp>

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace cxx {

// div

template <typename T> struct div_t {
  T quot;
  T rem;
};

template <typename T, typename U>
constexpr div_t<T> div_floor(const T x, const U y) {
  // x == q*y+r
  // q <= x/y
  // (q+1) > x/y
  // 0 <= r < y   | y > 0
  // 0 >= r < y   | y < 0
  T q = x / y;
  T r = x % y;
  if ((y > 0 && r < 0) || (y < 0 && r > 0)) {
    --q;
    r += y;
  }
  return {q, r};
}

template <typename T, typename U>
constexpr div_t<T> div_ceil(const T x, const U y) {
  // x == q*y+r
  // q <= x/y
  // (q-1) > x/y
  // 0 >= r > -y   | y > 0
  // 0 <= r < -y   | y < 0
  T q = x / y;
  T r = x % y;
  if ((y > 0 && r > 0) || (y < 0 && r < 0)) {
    ++q;
    r -= y;
  }
  return {q, r};
}

template <typename T, typename U>
constexpr div_t<T> div_exact(const T x, const U y) {
  // x == q*y+r
  // x == q*y
  T q = x / y;
  T r = x % y;
  cxx_assert(r == 0);
  return {q, r};
}

template <typename T, typename U, typename V>
constexpr T align_floor(const T x, const U y, const V m = T(0)) {
  return y > 0 ? div_floor(x + y - m, y).quot * y - y + m
               : div_ceil(x + y - m, y).quot * y - y + m;
}

template <typename T, typename U, typename V>
constexpr T align_ceil(const T x, const U y, const V m = T(0)) {
  return y > 0 ? div_ceil(x - m, y).quot * y + m
               : div_floor(x - m, y).quot * y + m;
}

// ipow

namespace detail {
template <typename T, typename U> constexpr T ipow_impl(T x, U y) {
  T r = y & 1 ? x : 1;
  while (y >>= 1) {
    x *= x;
    if (y & 1)
      r *= x;
  }
  return r;
}
}

template <typename T, typename U> constexpr T ipow(T x, U y) {
  if (y < 0)
    return T(1) / detail::ipow_impl(x, -y);
  return detail::ipow_impl(x, y);
}
}

#define CXX_CSTDLIB_HPP_DONE
#endif // #ifdef CXX_CSTDLIB_HPP
#ifndef CXX_CSTDLIB_HPP_DONE
#error "Cyclic include dependency"
#endif
