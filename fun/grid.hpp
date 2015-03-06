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

// fmapTopo

// template <typename F, typename G, template <typename> class C, typename T,
//           typename... Args, typename B = cxx::invoke_of_t<G, T,
//           std::ptrdiff_t>,
//           typename R = cxx::invoke_of_t<F, T, connectivity<B>, Args...>>
// auto fmapTopo(F &&f, G &&g, const adt::grid<C, T> &xs,
//               const connectivity<B> &bs, Args &&... args) {
//   return adt::grid<C, T>(typename adt::grid<C, T>::fmapTopo(),
//                          std::forward<F>(f), std::forward<G>(g), xs, bs,
//                          std::forward<Args>(args)...);
// }

// foldMap

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, std::ptrdiff_t D, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
             Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
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
decltype(auto) mextract(const adt::grid<C, T, D> &xs) {
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

template <template <typename> class C, typename R> C<R> mzero() {
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
