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

template <typename F, typename T, size_t N, typename... As,
          typename CT = std::array<T, N>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
C<R> fmap(const F &f, const std::array<T, N> &xs, const As &... as) {
  std::size_t s = xs.size();
  C<R> rs;
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], as...);
  return rs;
}

template <typename F, typename T, size_t N, typename T2, typename... As,
          typename CT = std::array<T, N>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap2(const F &f, const std::array<T, N> &xs, const std::array<T2, N> &ys,
           const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
  C<R> rs;
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], as...);
  return rs;
}

template <typename F, typename T, size_t N, typename T2, typename T3,
          typename... As, typename CT = std::array<T, N>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, T3, As...>::type>
C<R> fmap3(const F &f, const std::array<T, N> &xs, const std::array<T2, N> &ys,
           const std::array<T3, N> &zs, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
  C<R> rs;
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], as...);
  return rs;
}

// function

//    fmap: (a -> b) -> (r -> a) -> r -> b
//    fmap f g = \x -> f (g x)

template <typename F, typename T, typename A, typename CT = std::function<T(A)>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, const std::function<T(A)> &g) {
  return C<R>([f, g](const typename C<T>::argument_type &x) {
    return cxx::invoke(f, cxx::invoke(g, x));
  });
}

// list

template <typename F, typename T, typename Allocator,
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

template <typename F, typename T, typename Compare, typename Allocator,
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

template <typename F, typename T, typename... As,
          typename CT = std::shared_ptr<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
C<R> fmap(const F &f, const std::shared_ptr<T> &xs, const As &... as) {
  bool s = bool(xs);
  if (s == false)
    return std::make_shared<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, as...));
}

template <typename F, typename T, typename T2, typename... As,
          typename CT = std::shared_ptr<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap2(const F &f, const std::shared_ptr<T> &xs,
           const std::shared_ptr<T2> &ys, const As &... as) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (s == false)
    return std::make_shared<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, &ys, as...));
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
template <typename F, typename T, typename Allocator, typename... As,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
C<R> fmap(const F &f, const std::vector<T, Allocator> &xs, const As &... as) {
  std::size_t s = xs.size();
  C<R> rs(s);
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], as...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename... As,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap2(const F &f, const std::vector<T, Allocator> &xs,
           const std::vector<T2, Allocator2> &ys, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
  C<R> rs(s);
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], as...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename T3, typename Allocator3, typename... As,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap3(const F &f, const std::vector<T, Allocator> &xs,
           const std::vector<T2, Allocator2> &ys,
           const std::vector<T3, Allocator3> &zs, const As &... as) {
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
  C<R> rs(s);
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], as...);
  return rs;
}

template <typename F, typename G, typename T, typename Allocator, typename B,
          typename CT = std::vector<T, Allocator>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, B, B>::type>
typename std::enable_if<
    std::is_same<
        typename std::decay<typename cxx::invoke_of<G, T, bool>::type>::type,
        B>::value,
    C<R> >::type
stencil_fmap(const F &f, const G &g, const std::vector<T, Allocator> &xs,
             const B &bm, const B &bp) {
  size_t s = xs.size();
  assert(s >= 2);
  C<R> rs(s);
  rs[0] = cxx::invoke(f, xs[0], bm, cxx::invoke(g, xs[1], false));
  for (size_t i = 1; i < s - 1; ++i)
    rs[i] = cxx::invoke(f, xs[i], cxx::invoke(g, xs[i - 1], true),
                        cxx::invoke(g, xs[i + 1], false));
  rs[s - 1] = cxx::invoke(f, xs[s - 1], cxx::invoke(g, xs[s - 2], true), bp);
  return rs;
}
}

#endif // #ifndef CXX_FUNCTOR_HH
