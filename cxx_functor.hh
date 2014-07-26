#ifndef CXX_FUNCTOR_HH
#define CXX_FUNCTOR_HH

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

// fmap: (a -> b) -> m a -> m b

// TODO: allow additional arguments for all types

// array
namespace detail {
template <typename T> struct unwrap_array {
  typedef T type;
  const T &operator()(const T &x, std::size_t i) const { return x; }
};
template <typename T, size_t N> struct unwrap_array<std::array<T, N> > {
  typedef T type;
  const T &operator()(const std::array<T, N> &x, std::size_t i) const {
    return x[i];
  }
};
}
template <typename T, size_t N, typename... As, typename F,
          typename CT = std::array<T, N>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_array<As>::type...>::type>
C<R> fmap(const F &f, const std::array<T, N> &xs, const As &... as) {
  std::size_t s = xs.size();
  C<R> rs;
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], detail::unwrap_array<As>()(as, i)...);
  return rs;
}

// function
//    fmap: (a -> b) -> (r -> a) -> r -> b
//    fmap f g = \x -> f (g x)
template <typename T, typename A, typename F, typename CT = std::function<T(A)>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, const std::function<T(A)> &g) {
  return C<R>([f, g](const typename C<T>::argument_type &x) {
    return cxx::invoke(f, cxx::invoke(g, x));
  });
}

// list
template <typename T, typename Allocator, typename F,
          typename CT = std::list<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, const std::list<T, Allocator> &xs) {
  C<R> rs;
  for (const auto &x : xs)
    rs.push_back(cxx::invoke(f, x));
  return rs;
}

// set
template <typename T, typename Compare, typename Allocator, typename F,
          typename CT = std::set<T, Compare, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, const std::set<T, Compare, Allocator> &xs) {
  C<R> rs;
  for (const auto &x : xs)
    rs.insert(cxx::invoke(f, x));
  return rs;
}

// shared_ptr
namespace detail {
template <typename T> struct unwrap_shared_ptr {
  typedef T type;
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_shared_ptr<std::shared_ptr<T> > {
  typedef T type;
  const T &operator()(const std::shared_ptr<T> &x, std::size_t i) const {
    return *x;
  }
};
}
template <typename T, typename... As, typename F,
          typename CT = std::shared_ptr<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_shared_ptr<As>::type...>::type>
C<R> fmap(const F &f, const std::shared_ptr<T> &xs, const As &... as) {
  if (!xs)
    return std::make_shared<R>();
  return std::make_shared<R>(
      cxx::invoke(f, *xs, detail::unwrap_shared_ptr<As>()(as)...));
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
template <typename T, typename Allocator, typename... As, typename F,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_vector<As>::type...>::type>
C<R> fmap(const F &f, const std::vector<T, Allocator> &xs, const As &... as) {
  std::size_t s = xs.size();
  C<R> rs;
  rs.reserve(s);
  for (std::size_t i = 0; i < s; ++i)
    rs.push_back(cxx::invoke(f, xs[i], detail::unwrap_vector<As>()(as, i)...));
  return rs;
}
}

#endif // #ifndef CXX_FUNCTOR_HH
