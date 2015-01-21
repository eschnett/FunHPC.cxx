#ifndef CXX_FOLDABLE_HH
#define CXX_FOLDABLE_HH

#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace cxx {

// fold: (a -> a -> a) -> a -> t a -> a
// foldMap: (a -> b) -> (b -> b -> b) -> b -> t a -> b
// foldMap2: (a -> b -> c) -> (c -> c -> c) -> c -> t a -> t b -> c

////////////////////////////////////////////////////////////////////////////////

// foldl: (a -> b -> a) -> a -> t b -> a
// foldl1: (a -> a -> a) -> t a -> a

// and: t bool -> bool
// or:  t bool -> bool
// any: (a -> bool) -> t a -> bool
// all: (a -> bool) -> t a -> bool

// toVector: t a -> [a]
// elem:     a -> t a -> bool
// notElem:  a -> t a -> bool
// find:     (a -> bool) -> t a -> a*

// TODO: allow additional arguments for all types
// TODO: introduce foldable2
// TODO: rename "catamorphism"?

////////////////////////////////////////////////////////////////////////////////

// array

template <typename Op, typename R, std::size_t N, typename... As>
auto fold(const Op &op, const R &z, const std::array<R, N> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), xs[i], as...);
  return r;
}

template <typename T, std::size_t N> const T &head(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs.front();
}
template <typename T, std::size_t N> const T &last(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename F, typename Op, typename R, typename T, std::size_t N,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const std::array<T, N> &xs,
             const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap(f, xs, as...));
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], as...));
  return r;
}

template <typename F, typename Op, typename R, typename T, typename T2,
          std::size_t N, typename... As>
auto foldMap2(const F &f, const Op &op, const R &z, const std::array<T, N> &xs,
              const std::array<T2, N> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap2(f, xs, ys, as...));
  std::size_t s = xs.size();
  assert(ys.size() == s);
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], as...));
  return r;
}

// list

template <typename Op, typename R, typename... As>
auto fold(const Op &op, const R &z, const std::list<R> &xs, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), x, as...);
  return r;
}

template <typename T> const T &head(const std::list<T> &xs) {
  assert(!xs.empty());
  return xs.front();
}
template <typename T> const T &last(const std::list<T> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename F, typename Op, typename R, typename T, typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const std::list<T> &xs,
             const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap(f, xs, as...));
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, x, as...));
  return r;
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
auto foldMap2(const F &f, const Op &op, const R &z, const std::list<T> &xs,
              const std::list<T2> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap2(f, xs, ys, as...));
  typename std::list<T>::const_iterator xi = xs.begin, xe = xs.end();
  typename std::list<T2>::const_iterator yi = ys.begin, ye = ys.end();
  R r(z);
  while (xi != xe && yi != ye)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xi++, *yi++, as...));
  assert(xi == xe && yi == ye);
  return r;
}

// set

template <typename Op, typename R, typename... As>
auto fold(const Op &op, const R &z, const std::set<R> &xs, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), x, as...);
  return r;
}

template <typename T> const T &head(const std::set<T> &xs) {
  assert(!xs.empty());
  return *xs.begin();
}
template <typename T> const T &last(const std::set<T> &xs) {
  assert(!xs.empty());
  return *xs.rbegin();
}

template <typename F, typename Op, typename R, typename T, typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const std::set<T> &xs,
             const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap(f, xs, as...));
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, x, as...));
  return r;
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
auto foldMap2(const F &f, const Op &op, const R &z, const std::set<T> &xs,
              const std::set<T2> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap2(f, xs, ys, as...));
  typename std::set<T>::const_iterator xi = xs.begin, xe = xs.end();
  typename std::set<T2>::const_iterator yi = ys.begin, ye = ys.end();
  R r(z);
  while (xi != xe && yi != ye)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xi++, *yi++, as...));
  assert(xi == xe && yi == ye);
  return r;
}

// shared_ptr

template <typename Op, typename R, typename... As>
auto fold(const Op &op, const R &z, const std::shared_ptr<R> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  bool s = bool(xs);
  if (!s)
    return z;
  return *xs;
}

template <typename T> const T &head(const std::shared_ptr<T> &xs) {
  assert(bool(xs));
  return *xs;
}
template <typename T> const T &last(const std::shared_ptr<T> &xs) {
  assert(bool(xs));
  return *xs;
}

template <typename F, typename Op, typename R, typename T, typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const std::shared_ptr<T> &xs,
             const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  if (!s)
    return z;
  return cxx::invoke(f, *xs, as...);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
auto foldMap2(const F &f, const Op &op, const R &z,
              const std::shared_ptr<T> &xs, const std::shared_ptr<T2> &ys,
              const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::size_t s = bool(xs);
  assert(bool(ys) == s);
  if (!s)
    return z;
  return cxx::invoke(f, *xs, *ys, as...);
}

// vector

template <typename Op, typename R, typename... As>
auto fold(const Op &op, const R &z, const std::vector<R> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), xs[i], as...);
  return r;
}

template <typename T> const T &head(const std::vector<T> &xs) {
  assert(!xs.empty());
  return xs.front();
}
template <typename T> const T &last(const std::vector<T> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename F, typename Op, typename R, typename T, typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const std::vector<T> &xs,
             const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap(f, xs, as...));
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], as...));
  return r;
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
auto foldMap2(const F &f, const Op &op, const R &z, const std::vector<T> &xs,
              const std::vector<T2> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  // If T is an asynchronous type, then call fmap on all elements
  // first to expose parallelism
  if (cxx::is_async<T>::value)
    return fold(op, z, fmap2(f, xs, ys, as...));
  std::size_t s = xs.size();
  assert(ys.size() == s);
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], as...));
  return r;
}

////////////////////////////////////////////////////////////////////////////////

// namespace foldable {

template <template <typename> class C> bool and_(const C<bool> &xs) {
  return fold(std::logical_and<bool>(), true, xs);
}

template <template <typename> class C> bool or_(const C<bool> &xs) {
  return fold(std::logical_or<bool>(), false, xs);
}

template <typename F, typename CT, typename T = typename CT::value_type>
auto any_of(const F &f, const CT &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T>, bool>::value, "");
  return foldMap(f, std::logical_or<bool>(), false, xs);
}

template <typename F, typename CT, typename T = typename CT::value_type>
auto all_of(const F &f, const CT &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T>, bool>::value, "");
  return foldMap(f, std::logical_and<bool>(), true, xs);
}

template <template <typename> class C, typename T>
std::vector<T> to_vector(const C<T> &xs) {
  // TODO: Use vector's monadic operations
  // TODO: Use foldl if present, since more efficient?
  return foldMap([](const T &x) { return std::vector<T>({ x }); },
                 [](const std::vector<T> &xs, const std::vector<T> &ys) {
                   std::vector<T> rs(xs);
                   rs.insert(rs.end(), ys.begin(), ys.end());
                   return rs;
                 },
                 std::vector<T>(), xs);
}

template <template <typename> class C, typename T>
bool elem(const T &x, const C<T> &ys) {
  return any_of([x](const T &y) { return x == y; }, ys);
}

template <template <typename> class C, typename T>
bool not_elem(const T &x, const C<T> &ys) {
  return all_of([x](const T &y) { return x != y; }, ys);
}

template <template <typename> class C, typename T, typename F>
const T *find(const F &f, const C<T> &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T>, bool>::value, "");
  // TODO: Use ptr's monadic operations
  // TODO: Use foldl if present, since more efficient?
  return foldMap([f](const T &x) { return cxx::invoke(f, x) ? &x : nullptr; },
                 [](const T *r1, const T *r2) { return r1 ? r1 : r2; }, nullptr,
                 xs);
}
// }
}

#define CXX_FOLDABLE_HH_DONE
#else
#ifndef CXX_FOLDABLE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_FOLDABLE_HH
