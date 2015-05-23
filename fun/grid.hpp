#ifndef FUN_GRID_HPP
#define FUN_GRID_HPP

#include <adt/dummy.hpp>
#include <adt/grid.hpp>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_grid

namespace detail {
template <typename> struct is_grid : std::false_type {};
template <typename C, typename T, std::ptrdiff_t D>
struct is_grid<adt::grid<C, T, D>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename C, typename T, std::ptrdiff_t D>
struct fun_traits<adt::grid<C, T, D>> {
  template <typename U> using constructor = adt::grid<C, U, D>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = D;
  typedef typename adt::grid<C, T, D>::index_type index_type;
  typedef adt::grid<C, adt::dummy, D - 1> boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR(typename CR::iotaMap(), std::forward<F>(f), s,
            std::forward<Args>(args)...);
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr>
auto iotaMap(F &&f, const typename fun_traits<C>::index_type &s,
             Args &&... args) {
  typedef typename fun_traits<C>::index_type Index;
  typedef cxx::invoke_of_t<F, Index, Args...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR(typename CR::iotaMap(), std::forward<F>(f), s,
            std::forward<Args>(args)...);
}

// fmap

template <typename F, typename C, typename T, std::ptrdiff_t D,
          typename... Args>
auto fmap(F &&f, const adt::grid<C, T, D> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  return adt::grid<C, R, D>(typename adt::grid<C, R, D>::fmap(),
                            std::forward<F>(f), xs,
                            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, std::ptrdiff_t D, typename T2,
          typename... Args>
auto fmap2(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
           Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  return adt::grid<C, R, D>(typename adt::grid<C, R, D>::fmap2(),
                            std::forward<F>(f), xs, ys,
                            std::forward<Args>(args)...);
}

// boundary

template <typename C, typename T, std::ptrdiff_t D,
          std::enable_if_t<D != 0> * = nullptr>
auto boundary(const adt::grid<C, T, D> &xs, std::ptrdiff_t i) {
  typedef typename fun_traits<adt::grid<C, T, D>>::boundary_dummy BC;
  typedef typename fun_traits<BC>::template constructor<T> BCT;
  return BCT(typename BCT::boundary(), xs, i);
}

// boundaryMap

template <typename F, typename C, typename T, std::ptrdiff_t D,
          typename... Args, std::enable_if_t<D != 0> * = nullptr>
auto boundaryMap(F &&f, const adt::grid<C, T, D> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), std::forward<Args>(args)...);
}

// fmapStencil

template <typename F, typename G, typename C, typename T, typename... Args>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 0> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<G, T, std::ptrdiff_t> B __attribute__((__unused__));
  typedef cxx::invoke_of_t<F, T, std::size_t, Args...> R;
  return adt::grid<C, R, 0>(typename adt::grid<C, R, 0>::fmapStencil(),
                            std::forward<F>(f), std::forward<G>(g), xs,
                            std::forward<Args>(args)...);
}

template <typename F, typename G, typename C, typename T, typename BM0,
          typename BP0, typename... Args>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 1> &xs, BM0 &&bm0,
                 BP0 &&bp0, Args &&... args) {
  typedef cxx::invoke_of_t<G, T, std::ptrdiff_t> B;
  typedef cxx::invoke_of_t<F, T, std::size_t, B, B, Args...> R;
  static_assert(std::is_same<std::decay_t<BM0>, adt::grid<C, B, 0>>::value, "");
  static_assert(std::is_same<std::decay_t<BP0>, adt::grid<C, B, 0>>::value, "");
  return adt::grid<C, R, 1>(typename adt::grid<C, R, 1>::fmapStencil(),
                            std::forward<F>(f), std::forward<G>(g), xs,
                            std::forward<BM0>(bm0), std::forward<BP0>(bp0),
                            std::forward<Args>(args)...);
}

template <typename F, typename G, typename C, typename T, typename BM0,
          typename BM1, typename BP0, typename BP1, typename... Args>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 2> &xs, BM0 &&bm0,
                 BM1 &&bm1, BP0 &&bp0, BP1 &&bp1, Args &&... args) {
  typedef cxx::invoke_of_t<G, T, std::ptrdiff_t> B;
  typedef cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...> R;
  static_assert(std::is_same<std::decay_t<BM0>, adt::grid<C, B, 1>>::value, "");
  static_assert(std::is_same<std::decay_t<BM1>, adt::grid<C, B, 1>>::value, "");
  static_assert(std::is_same<std::decay_t<BP0>, adt::grid<C, B, 1>>::value, "");
  static_assert(std::is_same<std::decay_t<BP1>, adt::grid<C, B, 1>>::value, "");
  return adt::grid<C, R, 2>(typename adt::grid<C, R, 2>::fmapStencil(),
                            std::forward<F>(f), std::forward<G>(g), xs,
                            std::forward<BM0>(bm0), std::forward<BM1>(bm1),
                            std::forward<BP0>(bp0), std::forward<BP1>(bp1),
                            std::forward<Args>(args)...);
}

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          std::ptrdiff_t D, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
             Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename C, typename T,
          std::ptrdiff_t D, typename T2, typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
              const adt::grid<C, T2, D> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef std::decay_t<T> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  typedef typename C::container_dummy A;
  return CR(adt::set<typename fun_traits<C>::index_type>(1),
            munit<A>(std::forward<T>(x)));
}

// // mjoin
//
// template <typename C, typename T>
// adt::grid<C, T> mjoin(const adt::grid<C, adt::grid<C, T>> &xss) {
//   return adt::grid<C, T>(typename adt::grid<C, T>::join(), xss);
// }
//
// // mbind
//
// template <typename F, typename C, typename T, typename... Args,
//           typename CR = cxx::invoke_of_t<F, T, Args...>,
//           typename R = typename CR::value_type>
// adt::grid<C, R> mbind(F &&f, const adt::grid<C, T> &xs, Args &&... args) {
//   return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
// }

// mextract

template <typename C, typename T, std::ptrdiff_t D>
auto mextract(const adt::grid<C, T, D> &xs) {
  return xs.head();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          std::ptrdiff_t D, typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::grid<A, T, D> &xs,
              Args &&... args) {
  typedef typename fun_traits<adt::grid<A, T, D>>::dummy C;
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr>
auto mzero() {
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR();
}

// // mplus
//
// template <typename C, typename T, typename... Ts>
// adt::grid<C, T> mplus(const adt::grid<C, T> &xss,
//                       const adt::grid<C, Ts> &... yss) {
//   return adt::grid<C, T>(xss, yss...);
// }
//
// // msome
//
// template <typename C, typename T, typename... Ts,
//           typename R = std::decay_t<T>,
//           std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
// C<R> msome(T &&x, Ts &&... ys) {
//   return C<R>{std::forward<T>(x), std::forward<Ts>(ys)...};
// }

// mempty

template <typename C, typename T, std::ptrdiff_t D>
bool mempty(const adt::grid<C, T, D> &xs) {
  return xs.empty();
}

// msize

template <typename C, typename T, std::ptrdiff_t D>
std::size_t msize(const adt::grid<C, T, D> &xs) {
  return xs.size();
}
}

#define FUN_GRID_HPP_DONE
#endif // #ifdef FUN_GRID_HPP
#ifndef FUN_GRID_HPP_DONE
#error "Cyclic include dependency"
#endif
