#ifndef CXX_FOLDABLE_HH
#define CXX_FOLDABLE_HH

#include "cxx_invoke.hh"

#include <array>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>

namespace cxx {

// foldl: (a -> b -> a) -> a -> t b -> a

// and: t bool -> bool
// or:  t bool -> bool
// any: (a -> bool) -> t a -> bool
// all: (a -> bool) -> t a -> bool

// toVector: t a -> [a]
// elem:     a -> t a -> bool
// notElem:  a -> t a -> bool
// find:     (a -> bool) -> t a -> a*

namespace foldable {

template <typename R, typename T, size_t N, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const std::array<T, N> &xs) {
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x);
  return r;
}

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const std::list<T> &xs) {
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x);
  return r;
}

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const std::shared_ptr<T> &xs) {
  return !xs ? z : cxx::invoke(f, z, *xs);
}

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const std::set<T> &xs) {
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x);
  return r;
}

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const std::vector<T> &xs) {
  R r(z);
  for (const auto &x : xs)
    r = cxx::invoke(f, std::move(r), x);
  return r;
}
}

namespace foldable {

template <template <typename> class C> bool and_(const C<bool> &xs) {
  return foldl(std::logical_and<bool>(), true, xs);
}

template <template <typename> class C> bool or_(const C<bool> &xs) {
  return foldl(std::logical_or<bool>(), false, xs);
}

template <template <typename> class C, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T>::type, bool>::value, bool>::type
any(const F &f, const C<T> &xs) {
  return foldl([f](bool x, const T &y) { return x || cxx::invoke(f, y); },
               false, xs);
}

template <template <typename> class C, typename T, typename F>
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
