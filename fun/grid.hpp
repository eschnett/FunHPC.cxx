#ifndef FUN_GRID_HPP
#define FUN_GRID_HPP

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
template <template <typename> class C, typename T, std::ptrdiff_t D>
struct is_grid<adt::grid<C, T, D>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <template <typename> class C, typename T, std::ptrdiff_t D>
struct fun_traits<adt::grid<C, T, D>> {
  template <typename U> using constructor = adt::grid<C, U, D>;
  typedef T value_type;
  static constexpr std::ptrdiff_t rank = D;
  typedef typename adt::grid<C, T, D>::index_type index_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return C<R>(typename C<R>::iotaMap(), std::forward<F>(f), s,
              std::forward<Args>(args)...);
}

template <template <typename> class C, typename F, typename... Args,
          typename Ind = typename fun_traits<C<int>>::index_type,
          typename R = cxx::invoke_of_t<F, Ind, Args...>,
          std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, const Ind &s, Args &&... args) {
  return C<R>(typename C<R>::iotaMap(), std::forward<F>(f), s,
              std::forward<Args>(args)...);
}

// fmap

template <typename F, template <typename> class C, typename T, std::ptrdiff_t D,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const adt::grid<C, T, D> &xs, Args &&... args) {
  return adt::grid<C, R, D>(typename adt::grid<C, R, D>::fmap(),
                            std::forward<F>(f), xs,
                            std::forward<Args>(args)...);
}

template <typename F, template <typename> class C, typename T, std::ptrdiff_t D,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
           Args &&... args) {
  return adt::grid<C, R, D>(typename adt::grid<C, R, D>::fmap2(),
                            std::forward<F>(f), xs, ys,
                            std::forward<Args>(args)...);
}

// boundary

template <template <typename> class C, typename T, std::ptrdiff_t D,
          std::enable_if_t<D != 0> * = nullptr>
auto boundary(const adt::grid<C, T, D> &xs, std::ptrdiff_t i) {
  return adt::grid<C, T, D - 1>(typename adt::grid<C, T, D - 1>::boundary(), xs,
                                i);
}

// boundaryMap

template <typename F, template <typename> class C, typename T, std::ptrdiff_t D,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>,
          std::enable_if_t<D != 0> * = nullptr>
auto boundaryMap(F &&f, const adt::grid<C, T, D> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), std::forward<Args>(args)...);
}

// fmapStencil

template <typename F, typename G, template <typename> class C, typename T,
          typename... Args, typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, Args...>>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 0> &xs, Args &&... args) {
  return adt::grid<C, R, 0>(typename adt::grid<C, R, 0>::fmapStencil(),
                            std::forward<F>(f), std::forward<G>(g), xs,
                            std::forward<Args>(args)...);
}

template <typename F, typename G, template <typename> class C, typename T,
          typename BM0, typename BP0, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 1> &xs, BM0 &&bm0,
                 BP0 &&bp0, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM0>, adt::grid<C, B, 0>>::value, "");
  static_assert(std::is_same<std::decay_t<BP0>, adt::grid<C, B, 0>>::value, "");
  return adt::grid<C, R, 1>(typename adt::grid<C, R, 1>::fmapStencil(),
                            std::forward<F>(f), std::forward<G>(g), xs,
                            std::forward<BM0>(bm0), std::forward<BP0>(bp0),
                            std::forward<Args>(args)...);
}

template <typename F, typename G, template <typename> class C, typename T,
          typename BM0, typename BM1, typename BP0, typename BP1,
          typename... Args, typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>>
auto fmapStencil(F &&f, G &&g, const adt::grid<C, T, 2> &xs, BM0 &&bm0,
                 BM1 &&bm1, BP0 &&bp0, BP1 &&bp1, Args &&... args) {
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

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, std::ptrdiff_t D, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
             Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, std::ptrdiff_t D, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
              const adt::grid<C, T2, D> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
C<R> munit(T &&x) {
  return C<R>(adt::array_fill<std::ptrdiff_t, fun_traits<C<R>>::rank>(1),
              munit<C<R>::template storage_constructor>(std::forward<T>(x)));
}

// // mjoin
//
// template <template <typename> class C, typename T>
// adt::grid<C, T> mjoin(const adt::grid<C, adt::grid<C, T>> &xss) {
//   return adt::grid<C, T>(typename adt::grid<C, T>::join(), xss);
// }
//
// // mbind
//
// template <typename F, template <typename> class C, typename T, typename...
// Args,
//           typename CR = cxx::invoke_of_t<F, T, Args...>,
//           typename R = typename CR::value_type>
// adt::grid<C, R> mbind(F &&f, const adt::grid<C, T> &xs, Args &&... args) {
//   return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
// }

// mextract

template <template <typename> class C, typename T, std::ptrdiff_t D>
auto mextract(const adt::grid<C, T, D> &xs) {
  return xs.head();
}

// mfoldMap

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, std::ptrdiff_t D, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
adt::grid<C, R, D> mfoldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
                            Args &&... args) {
  return munit<fun_traits<adt::grid<C, T, D>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
auto mzero() {
  return C<R>();
}

// // mplus
//
// template <template <typename> class C, typename T, typename... Ts>
// adt::grid<C, T> mplus(const adt::grid<C, T> &xss,
//                       const adt::grid<C, Ts> &... yss) {
//   return adt::grid<C, T>(xss, yss...);
// }
//
// // msome
//
// template <template <typename> class C, typename T, typename... Ts,
//           typename R = std::decay_t<T>,
//           std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
// C<R> msome(T &&x, Ts &&... ys) {
//   return C<R>{std::forward<T>(x), std::forward<Ts>(ys)...};
// }

// mempty

template <template <typename> class C, typename T, std::ptrdiff_t D>
bool mempty(const adt::grid<C, T, D> &xs) {
  return xs.empty();
}
}

#define FUN_GRID_HPP_DONE
#endif // #ifdef FUN_GRID_HPP
#ifndef FUN_GRID_HPP_DONE
#error "Cyclic include dependency"
#endif
