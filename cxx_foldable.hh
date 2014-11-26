#ifndef CXX_FOLDABLE_HH
#define CXX_FOLDABLE_HH

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

////////////////////////////////////////////////////////////////////////////////

// array

template <typename Op, typename R, std::size_t N, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, const R &z, const std::array<R, N> &xs, const As &... as) {
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), xs[i], as...);
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, std::size_t N,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const std::array<T, N> &xs,
        const As &... as) {
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], as...));
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          std::size_t N, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, const R &z, const std::array<T, N> &xs,
         const std::array<T2, N> &ys, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], as...));
  return std::move(r);
}

// list

template <typename Op, typename R, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, R r, const std::list<R> &xs, const As &... as) {
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), x, as...);
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, R r, const std::list<T> &xs,
        const As &... as) {
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, x, as...));
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, R r, const std::list<T> &xs,
         const std::list<T2> &ys, const As &... as) {
  typename std::list<T>::const_iterator xi = xs.begin, xe = xs.end();
  typename std::list<T2>::const_iterator yi = ys.begin, ye = ys.end();
  while (xi != xe && yi != ye)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xi++, *yi++, as...));
  assert(xi == xe && yi == ye);
  return std::move(r);
}

// set

template <typename Op, typename R, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, R r, const std::set<R> &xs, const As &... as) {
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), x, as...);
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, R r, const std::set<T> &xs,
        const As &... as) {
  for (const auto &x : xs)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, x, as...));
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, R r, const std::set<T> &xs,
         const std::set<T2> &ys, const As &... as) {
  typename std::set<T>::const_iterator xi = xs.begin, xe = xs.end();
  typename std::set<T2>::const_iterator yi = ys.begin, ye = ys.end();
  while (xi != xe && yi != ye)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xi++, *yi++, as...));
  assert(xi == xe && yi == ye);
  return std::move(r);
}

// shared_ptr

template <typename Op, typename R, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, R r, const std::shared_ptr<R> &xs, const As &... as) {
  bool s = bool(xs);
  if (s)
    r = cxx::invoke(op, std::move(r), *xs, as...);
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, R r, const std::shared_ptr<T> &xs,
        const As &... as) {
  bool s = bool(xs);
  if (s)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xs, as...));
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, R r, const std::shared_ptr<T> &xs,
         const std::shared_ptr<T2> &ys, const As &... as) {
  std::size_t s = bool(xs);
  assert(bool(ys) == s);
  if (s)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, *xs, *ys, as...));
  return std::move(r);
}

// vector

template <typename Op, typename R, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, const R &z, const std::vector<R> &xs, const As &... as) {
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), xs[i], as...);
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const std::vector<T> &xs,
        const As &... as) {
  std::size_t s = xs.size();
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], as...));
  return std::move(r);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, const R &z, const std::vector<T> &xs,
         const std::vector<T2> &ys, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
#pragma omp declare reduction(op : R : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
  R r(z);
#pragma omp simd reduction(op : r)
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], as...));
  return std::move(r);
}

////////////////////////////////////////////////////////////////////////////////

#if 0

// array

template <typename F, typename R, typename T, std::size_t N, typename... As,
          typename CT = std::array<T, N>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::array<T, N> &xs, const As &... as) {
  std::size_t s = xs.size();
    for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], as...);
  return r;
}

template <typename F, typename R, typename T, std::size_t N, typename T2,
          typename... As, typename CT = std::array<T, N>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const std::array<T, N> &xs,
       const std::array<T2, N> &ys, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
    for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], ys[i], as...);
  return r;
}

template <typename F, typename R, typename T, std::size_t N, typename T2,
          typename T3, typename... As, typename CT = std::array<T, N>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, T3, As...>::type,
                 R>::value,
    R>::type
foldl3(const F &f, const R &z, const std::array<T, N> &xs,
       const std::array<T2, N> &ys, const std::array<T3, N> &zs,
       const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
    for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], ys[i], zs[i], as...);
  return r;
}

template <typename F, typename T, std::size_t N, typename... As,
          typename CT = std::array<T, N>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, T, As...>::type, T>::value,
    T>::type
foldl1(const F &f, const std::array<T, N> &xs, const As &... as) {
  std::size_t s = xs.size();
  assert(s != 0);
  T r(xs[0]);
  for (std::size_t i = 1; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], as...);
  return r;
}

// list

template <typename F, typename R, typename T, typename Allocator,
          typename... As, typename CT = std::list<T, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::list<T, Allocator> &xs,
      const As &... as) {
    for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x, as...);
  return r;
}

template <typename F, typename T, typename Allocator, typename... As,
          typename CT = std::list<T, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, T, As...>::type, T>::value,
    T>::type
foldl1(const F &f, const std::list<T, Allocator> &xs, const As &... as) {
  assert(!xs.empty());
  auto i = xs.cbegin();
  T r(*i++);
  while (i != xs.cend())
    r = cxx::invoke(f, std::move(r), *i++, as...);
  return r;
}

// set

template <typename F, typename R, typename T, typename Compare,
          typename Allocator, typename... As,
          typename CT = std::set<T, Compare, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::set<T, Compare, Allocator> &xs,
      const As &... as) {
    for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x, as...);
  return r;
}

template <typename F, typename T, typename Compare, typename Allocator,
          typename... As, typename CT = std::set<T, Compare, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, T, As...>::type, T>::value,
    T>::type
foldl1(const F &f, const std::set<T, Compare, Allocator> &xs,
       const As &... as) {
  assert(!xs.empty());
  auto i = xs.cbegin();
  T r(*i++);
  while (i != xs.cend())
    r = cxx::invoke(f, std::move(r), *i++, as...);
  return r;
}

// shared_ptr

template <typename F, typename R, typename T, typename... As,
          typename CT = std::shared_ptr<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::shared_ptr<T> &xs, const As &... as) {
  return !xs ? z : cxx::invoke(f, z, *xs, as...);
}

template <typename F, typename T, typename... As,
          typename CT = std::shared_ptr<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, T, As...>::type, T>::value,
    T>::type
foldl1(const F &f, const std::shared_ptr<T> &xs, const As &... as) {
  assert(!xs.empty());
  return *xs;
}

// vector

template <typename F, typename R, typename T, typename Allocator,
          typename... As, typename CT = std::vector<T, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::vector<T, Allocator> &xs,
      const As &... as) {
  std::size_t s = xs.size();
    for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], as...);
  return std::move(r);
}

template <typename F, typename R, typename T, typename Allocator, typename T2,
          typename Allocator2, typename... As,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const std::vector<T, Allocator> &xs,
       const std::vector<T2, Allocator2> &ys, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
    for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], ys[i], as...);
  return std::move(r);
}

template <typename F, typename T, typename Allocator, typename... As,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, T, As...>::type, T>::value,
    T>::type
foldl1(const F &f, const std::vector<T, Allocator> &xs, const As &... as) {
  std::size_t s = xs.size();
  assert(s != 0);
  T r(xs[0]);
  for (std::size_t i = 1; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], as...);
  return std::move(r);
}

#endif

////////////////////////////////////////////////////////////////////////////////

namespace foldable {

template <template <typename> class C> bool and_(const C<bool> &xs) {
  return fold(std::logical_and<bool>(), true, xs);
}

template <template <typename> class C> bool or_(const C<bool> &xs) {
  return fold(std::logical_or<bool>(), false, xs);
}

template <typename F, typename CT, typename T = typename CT::value_type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value, bool>::type
any_of(const F &f, const CT &xs) {
  return foldMap(f, std::logical_or<bool>(), false, xs);
}

template <typename F, typename CT, typename T = typename CT::value_type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value, bool>::type
all_of(const F &f, const CT &xs) {
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
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value,
    const T *>::type
find(const F &f, const C<T> &xs) {
  // TODO: Use ptr's monadic operations
  // TODO: Use foldl if present, since more efficient?
  return foldMap([f](const T &x) { return cxx::invoke(f, x) ? &x : nullptr; },
                 [](const T *r1, const T *r2) { return r1 ? r1 : r2; }, nullptr,
                 xs);
}
}
}

#endif // #ifndef CXX_FOLDABLE_HH
