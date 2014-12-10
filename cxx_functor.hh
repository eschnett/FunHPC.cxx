#ifndef CXX_FUNCTOR_HH
#define CXX_FUNCTOR_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_shape_fwd.hh"
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

template <typename F, typename G, typename T, size_t N, typename B,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const std::array<T, N> &xs,
                  const B &bm, const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<G, T, bool> >, B>::value, "");
  size_t s = xs.size();
  std::array<R, N> rs(s);
  if (s == 1) {
    rs[0] = cxx::invoke(f, xs[0], bm, bp, as...);
  } else if (s > 1) {
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

template <typename F, typename T, std::size_t N, typename... As>
auto boundary(const F &f, const std::array<T, N> &xs, std::ptrdiff_t dir,
              bool face, const As &... as) {
  typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;
  assert(dir == 0);
  return std::array<R, 1>{ cxx::invoke(f, !face ? head(xs) : last(xs), dir,
                                       face, as...) };
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

template <typename F, typename T, typename... As>
auto fmap(const F &f, const std::list<T> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  std::list<R> rs;
  for (const auto &x : xs)
    rs.push_back(cxx::invoke(f, x, as...));
  return rs;
}

template <typename F, typename G, typename T, typename B, typename... As>
auto stencil_fmap(const F &f, const G &g, const std::list<T> &xs, const B &bm,
                  const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<G, T, bool> >, B>::value, "");
  std::list<R> rs;
  for (auto b = xs.begin(), e = xs.end(), i = b; i != e; ++i)
    rs.push_back(cxx::invoke(
        f, *i, i == b ? bm : cxx::invoke(g, *prev(i), true),
        next(i) == e ? bp : cxx::invoke(g, *next(i), false), as...));
  return rs;
}

template <typename F, typename T, typename... As>
auto boundary(const F &f, const std::list<T> &xs, std::ptrdiff_t dir, bool face,
              const As &... as) {
  assert(dir == 0);
  return std::list<T>{ cxx::invoke(f, !face ? head(xs) : last(xs), dir, face,
                                   as...) };
}

// set

template <typename F, typename T, typename Compare, typename Allocator,
          typename... As>
auto fmap(const F &f, const std::set<T, Compare, Allocator> &xs,
          const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  std::set<R /*Compare, Allocator*/> rs;
  for (const auto &x : xs)
    rs.insert(cxx::invoke(f, x, as...));
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
    return std::shared_ptr<R>();
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
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(f, *xs, *ys, *zs, as...));
}

template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
auto fmap_boundaries(const F &f, const std::shared_ptr<T> &xs,
                     const boundaries<std::shared_ptr<B>, D> &bss,
                     const As &... as) {
  typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
  bool s = bool(xs);
  if (s == false)
    return std::shared_ptr<R>();
  assert(foldMap([](const std::shared_ptr<B> &bs) { return bool(bs); },
                 std::logical_and<bool>(), true, bss));
  return std::make_shared<R>(cxx::invoke(
      f, *xs, fmap([](const std::shared_ptr<B> &bs) { return *bs; }, bss),
      as...));
}

// template <typename F, typename G, typename T, typename B, typename... As>
// auto stencil_fmap(const F &f, const G &g, const std::shared_ptr<T> &xs,
//                   const B &bm, const B &bp, const As &... as) {
//   typedef cxx::invoke_of_t<F, T, B, B, As...> R;
//   static_assert(
//       std::is_same<std::decay_t<cxx::invoke_of_t<G, T, bool> >, B>::value,
//       "");
//   bool s = bool(xs);
//   if (s == false)
//     return std::shared_ptr<R>();
//   return cxx::fmap(f, xs, bm, bp, as...);
// }

// template <typename F, typename T, typename... As>
// auto boundary(const F &f, const std::shared_ptr<T> &xs, std::ptrdiff_t dir,
//               bool face, const As &... as) {
//   assert(bool(xs));
//   return fmap(f, xs, dir, face, as...);
// }

// template <typename F, typename G, typename T, typename B, std::ptrdiff_t D,
//           typename... As>
// auto stencil_fmap(const F &f, const G &g, const std::shared_ptr<T> &xs,
//                   const boundaries<std::shared_ptr<B>, D> &bs,
//                   const As &... as) {
//   typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
//   static_assert(
//       std::is_same<std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>
//       >,
//                    B>::value,
//       "");
//   bool s = bool(xs);
//   if (s == false)
//     return std::shared_ptr<R>();
//   assert(foldMap([](const auto &b) { return bool(b); },
//                  std::logical_and<bool>(), true, bs));
//   return cxx::fmap(f, xs, fmap([](const auto &b) { return *b; }, bs), as...);
// }

// vector

template <typename F, typename T, typename... As>
auto fmap(const F &f, const std::vector<T> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  std::size_t s = xs.size();
  std::vector<R> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], as...);
  return rs;
}

template <typename F, typename T, typename T2, typename... As>
auto fmap2(const F &f, const std::vector<T> &xs, const std::vector<T2> &ys,
           const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  std::vector<R> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], as...);
  return rs;
}

template <typename F, typename T, typename T2, typename T3, typename... As>
auto fmap3(const F &f, const std::vector<T> &xs, const std::vector<T2> &ys,
           const std::vector<T3> &zs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;
  std::size_t s = xs.size();
  assert(ys.size() == s);
  assert(zs.size() == s);
  std::vector<R> rs(s);
#pragma omp simd
  for (std::size_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], as...);
  return rs;
}

template <typename F, typename G, typename T, typename B, typename... As>
auto stencil_fmap(const F &f, const G &g, const std::vector<T> &xs, const B &bm,
                  const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<G, T, bool> >, B>::value, "");
  size_t s = xs.size();
  std::vector<R> rs(s);
  if (s == 1) {
    rs[0] = cxx::invoke(f, xs[0], bm, bp, as...);
  } else if (s > 1) {
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

template <typename F, typename T, typename... As>
auto boundary(const F &f, const std::vector<T> &xs, std::ptrdiff_t dir,
              bool face, const As &... as) {
  typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;
  assert(dir == 0);
  return std::vector<R>{ cxx::invoke(f, !face ? xs.front() : xs.back(), dir,
                                     face, as...) };
}

template <typename F, typename G, typename T, typename B, std::ptrdiff_t D,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const std::vector<T> &xs,
                  const boundaries<std::vector<B>, D> &bs, const As &... as) {
  static_assert(D == 1, "");
  typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool> >,
                   B>::value,
      "");
  size_t s = xs.size();
  std::vector<R> rs(s);
  auto makebnds = [](const B &bm, const B &bp) {
    return boundaries<B, D>(vect<B, D>::set1(bm), vect<B, D>::set1(bp));
  };
  if (s == 1) {
    rs[0] = cxx::invoke(
        f, xs[0], makebnds(bs(0, false).back(), bs(0, true).front()), as...);
  } else if (s > 1) {
    //     rs[0] = cxx::invoke(f, xs[0], bs(0, false).back(),
    //                         cxx::invoke(g, xs[1], 0, false), as...);
    // #pragma omp simd
    //     for (size_t i = 1; i < s - 1; ++i)
    //       rs[i] = cxx::invoke(f, xs[i], cxx::invoke(g, xs[i - 1], 0, true),
    //                           cxx::invoke(g, xs[i + 1], 0, false), as...);
    //     rs[s - 1] = cxx::invoke(f, xs[s - 1], cxx::invoke(g, xs[s - 2], 0,
    //     true),
    //                             bs(0, true).front(), as...);
  }
  return rs;
}
}

#define CXX_FUNCTOR_HH_DONE
#else
#ifndef CXX_FUNCTOR_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_FUNCTOR_HH
