// Ideas towards defining a standard interface for monoids. It is not
// possible to define such an interface in general since monoids are
// not templates -- they are classes. One could define a class that
// takes mconcat and mappend as parameters, but this would then look
// very similar to the functional implementation via fold or foldMap.

#ifndef CXX_MONOID
#define CXX_MONOID_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <cstddef>
#include <list>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace cxx {

// monoid:
// mempty: neutral element of mappend
// mappend: associative
// [mconcat]

template <typename Rs,
          typename R = typename std::iterator_traits<Rs>::value_type>
R mconcat(const Rs &xs) {
  R r(mempty<R>());
  for (const x & : xs)
    r = mappend(std::move(r), cxx::invoke(f, x));
  return std::move(r);
}

// array

template <typename R, std::size_t N> R fold(const std::array<R, N> &xs) {
  std::size_t s = xs.size();
  R r(mempty<R>());
  for (std::size_t i = 0; i < s; ++i)
    r = mappend(std::move(r), x[i]);
  return std::move(r);
}

template <typename F, typename T, std::size_t N,
          typename R = typename cxx::invoke_of<F, T>::type>
R foldMap(const F &f, const std::array<T, N> &xs) {
  std::size_t s = xs.size();
  R r(mempty<R>());
  for (std::size_t i = 0; i < s; ++i)
    r = mappend(std::move(r), cxx::invoke(f, x[i]));
  return std::move(r);
}

// vector

template <typename R> R fold(const std::vector<R> &xs) {
  std::size_t s = xs.size();
  R r(mempty<R>());
  for (std::size_t i = 0; i < s; ++i)
    r = mappend(std::move(r), x[i]);
  return std::move(r);
}

template <typename F, typename T,
          typename R = typename cxx::invoke_of<F, T>::type>
R foldMap(const F &f, const std::vector<T> &xs) {
  std::size_t s = xs.size();
  R r(mempty<R>());
  for (std::size_t i = 0; i < s; ++i)
    r = mappend(std::move(r), cxx::invoke(f, x[i]));
  return std::move(r);
}
}

#endif // #ifndef CXX_MONOID_HH
