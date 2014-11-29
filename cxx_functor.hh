#ifndef CXX_FUNCTOR_HH
#define CXX_FUNCTOR_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_utils.hh"

#include <cereal/access.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>
#include <tuple>
#include <type_traits>

namespace cxx {

// fmap: (a -> b) -> m a -> m b

// stencil_fmap: (a -> b -> b -> c) -> (a -> Bool -> c) -> m a -> b -> b -> m c

// Handling grid topologies:
// D: rank
// array<D>: shape
// function shape->size
// function index->offset
// boundary: rank D-1, array<D,2>
// F: interior -> boundary -> interior
// G: interior -> direction -> face -> boundary

////////////////////////////////////////////////////////////////////////////////

// array

template <typename F, typename T, size_t N, typename... As>
auto fmap(const F &f, const std::array<T, N> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  std::size_t s = xs.size();
  std::array<R, N> rs;
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], as...);
  return rs;
}

template <typename F, typename T, size_t N, typename T2, typename... As>
auto fmap2(const F &f, const std::array<T, N> &xs, const std::array<T2, N> &ys,
           const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  std::array<R, N> rs;
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], as...);
  return rs;
}

template <typename F, typename T, size_t N, typename T2, typename T3,
          typename... As>
auto fmap3(const F &f, const std::array<T, N> &xs, const std::array<T2, N> &ys,
           const std::array<T3, N> &zs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
  std::array<R, N> rs;
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], as...);
  return rs;
}

// function

//    fmap: (a -> b) -> (r -> a) -> r -> b
//    fmap f g = \x -> f (g x)

template <typename F, typename T, typename A>
auto fmap(const F &f, const std::function<T(A)> &g) {
  typedef cxx::invoke_of_t<F, T> R;
  return std::function<R(A)>(
      [f, g](const A &x) { return cxx::invoke(f, cxx::invoke(g, x)); });
}

// list

template <typename F, typename T, typename Allocator>
auto fmap(const F &f, const std::list<T, Allocator> &xs) {
  typedef cxx::invoke_of_t<F, T> R;
  std::list<R /*Allocator*/> rs;
  for (const auto &x : xs)
    rs.push_back(cxx::invoke(f, x));
  return rs;
}

// set

template <typename F, typename T, typename Compare, typename Allocator>
auto fmap(const F &f, const std::set<T, Compare, Allocator> &xs) {
  typedef cxx::invoke_of_t<F, T> R;
  std::set<R /*Compare, Allocator*/> rs;
  for (const auto &x : xs)
    rs.insert(cxx::invoke(f, x));
  return rs;
}

// shared_ptr

template <typename F, typename T, typename... As>
auto fmap(const F &f, const std::shared_ptr<T> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  bool s = bool(xs);
  if (s == false)
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, as...));
}

template <typename F, typename T, typename T2, typename... As>
auto fmap2(const F &f, const std::shared_ptr<T> &xs,
           const std::shared_ptr<T2> &ys, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, As...> R;
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (s == false)
    return std::make_shared<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, *ys, as...));
}

template <typename F, typename T, typename T2, typename T3, typename... As>
auto fmap3(const F &f, const std::shared_ptr<T> &xs,
           const std::shared_ptr<T2> &ys, const std::shared_ptr<T3> &zs,
           const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(bool(zs) == s);
  if (s == false)
    return std::make_shared<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, *ys, *zs, as...));
}

// vector

namespace detail {
template <typename T> struct unwrap_vector {
  typedef T type;
  const T &operator()(const T &x, std::size_t i) const { return x; }
};
template <typename T> struct unwrap_vector<std::vector<T> > {
  typedef T type;
  const T &operator()(const std::vector<T> &x, std::size_t i) const {
    return x[i];
  }
};
}
template <typename F, typename T, typename Allocator, typename... As>
auto fmap(const F &f, const std::vector<T, Allocator> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  std::size_t s = xs.size();
  std::vector<R /*Allocator*/> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], as...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename... As>
auto fmap2(const F &f, const std::vector<T, Allocator> &xs,
           const std::vector<T2, Allocator2> &ys, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  std::vector<R /*Allocator*/> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], as...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename T3, typename Allocator3, typename... As>
auto fmap3(const F &f, const std::vector<T, Allocator> &xs,
           const std::vector<T2, Allocator2> &ys,
           const std::vector<T3, Allocator3> &zs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
  std::vector<R /*Allocator*/> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], as...);
  return rs;
}

template <typename F, typename G, typename T, typename Allocator, typename B,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const std::vector<T, Allocator> &xs,
                  const B &bm, const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<G, T, bool> >, B>::value, "");
  size_t s = xs.size();
  assert(s >= 1);
  std::vector<R /* Allocator*/> rs(s);
  if (s == 1) {
    rs[0] = cxx::invoke(f, xs[0], bm, bp, as...);
  } else {
    rs[0] = cxx::invoke(f, xs[0], bm, cxx::invoke(g, xs[1], false), as...);
#pragma omp simd
    for (size_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], cxx::invoke(g, xs[i - 1], true),
                          cxx::invoke(g, xs[i + 1], false), as...);
    rs[s - 1] =
        cxx::invoke(f, xs[s - 1], cxx::invoke(g, xs[s - 2], true), bp, as...);
  }
  return rs;
}
}

#endif // #ifndef CXX_FUNCTOR_HH
