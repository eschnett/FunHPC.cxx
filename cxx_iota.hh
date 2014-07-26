#ifndef CXX_IOTA_HH
#define CXX_IOTA_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_utils.hh"

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>
#include <tuple>
#include <type_traits>

namespace cxx {

// iota: (Int -> a) ->  Int ->m a

// array
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_array<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  C<T> rs;
  for (std::ptrdiff_t i = imin; i < imax; i += istep)
    rs[i] = cxx::invoke(f, i, as...);
  return rs;
}

// function
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_function<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  return unit<C>(cxx::invoke(f, imin, as...));
}

// list
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_list<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  C<T> rs;
  for (std::ptrdiff_t i = imin; i < imax; i += istep)
    rs.push_back(cxx::invoke(f, i, as...));
  return rs;
}

// set
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_set<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  C<T> rs;
  for (std::ptrdiff_t i = imin; i < imax; i += istep)
    rs.insert(cxx::invoke(f, i, as...));
  return rs;
}

// shared_ptr
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  return unit<C>(cxx::invoke(f, imin, as...));
}

// vector
template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  C<T> rs;
  rs.reserve(cxx::div_ceil(imax - imin, istep));
  for (std::ptrdiff_t i = imin; i < imax; i += istep)
    rs.push_back(cxx::invoke(f, i, as...));
  return rs;
}
}

#endif // #ifndef CXX_IOTA_HH
