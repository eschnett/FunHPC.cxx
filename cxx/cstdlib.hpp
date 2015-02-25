#ifndef CXX_CSTDLIB_HPP
#define CXX_CSTDLIB_HPP

#include <cassert>

namespace cxx {

template <typename T, typename U> auto fld(T x, U y) {
  // x == r*y+m
  auto r = x / y;
  auto m = x % y;
  if (m < 0)
    --r;
  assert(r * y <= x && (r + 1) * y > x);
  return r;
}

template <typename T, typename U> auto cld(T x, U y) {
  // x == r*y+m
  auto r = x / y;
  auto m = x % y;
  if (m > 0)
    ++r;
  assert(r * y >= x && (r - 1) * y < x);
  return r;
}
}

#define CXX_CSTDLIB_HPP_DONE
#endif // #ifdef CXX_CSTDLIB_HPP
#ifndef CXX_CSTDLIB_HPP_DONE
#error "Cyclic include dependency"
#endif
