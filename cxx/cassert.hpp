#ifndef CXX_CASSERT_HPP
#define CXX_CASSERT_HPP

#include <cassert>

#ifndef NDEBUG
#define cxx_assert(expr) assert(expr)
#else
// #define cxx_assert(expr) __builtin_assume(expr)
#define cxx_assert(expr) assert(expr)
#endif

#define CXX_CASSERT_HPP_DONE
#endif // #ifdef CXX_CASSERT_HPP
#ifndef CXX_CASSERT_HPP_DONE
#error "Cyclic include dependency"
#endif
