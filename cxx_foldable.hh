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

// and: t bool -> bool
// or:  t bool -> bool
// any: (a -> bool) -> t a -> bool
// all: (a -> bool) -> t a -> bool

// toVector: t a -> [a]
// elem:     a -> t a -> bool
// notElem:  a -> t a -> bool
// find:     (a -> bool) -> t a -> a*

// Foldable API, to be implemented for all foldable type constructors

// template <typename CT> struct foldable {
//   template <typename R, typename F,
//             template <typename> class C = cxx::kinds<CT>::template
// constructor,
//             typename T = typename cxx::kinds<CT>::element_type>
//   static typename std::enable_if<
//       std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value,
// R>::type
//   foldl(const F &f, const R &z, const CT &xs);
// };
template <typename CT> struct foldable;

// Convenience functions

template <
    typename R, typename CT, typename F,
    /*TODO template <typename> class C = cxx::kinds<CT>::template constructor,*/
    typename T = typename cxx::kinds<CT>::element_type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const CT &xs) {
  return foldable<CT>::foldl(f, z, xs);
}

////////////////////////////////////////////////////////////////////////////////

// TODO: choose better namespace name
namespace foldable1 {

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

////////////////////////////////////////////////////////////////////////////////

// array
template <typename T, std::size_t N> struct foldable<std::array<T, N> > {
  template <typename R, typename F,
            template <typename> class C =
                cxx::kinds<std::array<T, N> >::template constructor>
  static typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z, const C<T> &xs) {
    R r(z);
    for (const auto &x : xs)
      r = cxx::invoke(f, std::move(r), x);
    return r;
  }
};

// list
template <typename T, typename Allocator>
struct foldable<std::list<T, Allocator> > {
  template <typename R, typename F,
            template <typename> class C =
                cxx::kinds<std::list<T, Allocator> >::template constructor>
  static typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z, const C<T> &xs) {
    R r(z);
    for (const auto &x : xs)
      r = cxx::invoke(f, std::move(r), x);
    return r;
  }
};

// set
template <typename T, typename Compare, typename Allocator>
struct foldable<std::set<T, Compare, Allocator> > {
  template <typename R, typename F,
            template <typename> class C = cxx::kinds<
                std::set<T, Compare, Allocator> >::template constructor>
  static typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z,
        const /*C<T>*/ std::set<T, Compare, Allocator> &xs) {
    R r(z);
    for (const auto &x : xs)
      r = cxx::invoke(f, std::move(r), x);
    return r;
  }
};

// shared_ptr
template <typename T> struct foldable<std::shared_ptr<T> > {
  template <typename R, typename F,
            template <typename> class C =
                cxx::kinds<std::shared_ptr<T> >::template constructor>
  static typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z, const C<T> &xs) {
    return !xs ? z : cxx::invoke(f, z, *xs);
  }
};

// vector
template <typename T, typename Allocator>
struct foldable<std::vector<T, Allocator> > {
  template <typename R, typename F,
            template <typename> class C =
                cxx::kinds<std::vector<T, Allocator> >::template constructor>
  static typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z, const /*C<T>*/ std::vector<T, Allocator> &xs) {
    R r(z);
    for (const auto &x : xs)
      r = cxx::invoke(f, std::move(r), x);
    return r;
  }
};
}

#endif // #ifndef CXX_FOLDABLE_HH
