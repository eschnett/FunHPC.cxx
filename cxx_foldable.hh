#ifndef CXX_FOLDABLE_HH
#define CXX_FOLDABLE_HH

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

// array

template <typename F, typename R, typename T, std::size_t N, typename... As,
          typename CT = std::array<T, N>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const std::array<T, N> &xs, const As &... as) {
  std::size_t s = xs.size();
  R r(z);
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
  R r(z);
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
  R r(z);
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
  R r(z);
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
  R r(z);
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
  std::cout << "vector::foldl.0\n";
  std::size_t s = xs.size();
  R r(z);
  for (std::size_t i = 0; i < s; ++i)
    r = cxx::invoke(f, std::move(r), xs[i], as...);
  std::cout << "vector::foldl.9\n";
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
  R r(z);
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

////////////////////////////////////////////////////////////////////////////////

namespace foldable {

template <template <typename> class C> bool and_(const C<bool> &xs) {
  return foldl(std::logical_and<bool>(), true, xs);
}

template <template <typename> class C> bool or_(const C<bool> &xs) {
  return foldl(std::logical_or<bool>(), false, xs);
}

template <template <typename> class C, typename F, typename T>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value, bool>::type
any(const F &f, const C<T> &xs) {
  return foldl([f](bool x, const T &y) { return x || cxx::invoke(f, y); },
               false, xs);
}

template <template <typename> class C, typename F, typename T>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value, bool>::type
all(const F &f, const C<T> &xs) {
  return foldl([f](bool x, const T &y) { return x && cxx::invoke(f, y); }, true,
               xs);
}

template <template <typename> class C, typename T>
std::vector<T> to_vector(const C<T> &xs) {
  std::vector<T> r;
  return foldl([](std::vector<T> &r, const T &x) {
                 r.push_back(x);
                 return r;
               },
               std::vector<T>(), xs);
}

template <template <typename> class C, typename T>
bool elem(const T &x, const C<T> &ys) {
  return any([x](const T &y) { return x == y; }, ys);
}

template <template <typename> class C, typename T>
bool not_elem(const T &x, const C<T> &ys) {
  return all([x](const T &y) { return x != y; }, ys);
}

template <template <typename> class C, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value,
    const T *>::type
find(const F &f, const C<T> &xs) {
  return foldl([f](const T *r, const T &x) {
                 return r ? r : cxx::invoke(f, x) ? &x : nullptr;
               },
               nullptr, xs);
}
}
}

#endif // #ifndef CXX_FOLDABLE_HH
