#ifndef FUN_GRID_IMPL_HPP
#define FUN_GRID_IMPL_HPP

#include "grid_decl.hpp"

#include <adt/grid_impl.hpp>

#include <cxx/cassert.hpp>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> *, typename R,
          typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  return CR(typename CR::iotaMap(), std::forward<F>(f), inds,
            std::forward<Args>(args)...);
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> *, typename R,
          typename CR>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  return CR(typename CR::iotaMapMulti(), std::forward<F>(f), inds,
            std::forward<Args>(args)...);
}

// fmap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          typename CT, typename R, typename CR>
CR fmap(F &&f, const adt::grid<C, T, D> &xs, Args &&... args) {
  return CR(typename CR::fmap(), std::forward<F>(f), xs,
            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename... Args, typename CT, typename R, typename CR>
CR fmap2(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
         Args &&... args) {
  return CR(typename CR::fmap2(), std::forward<F>(f), xs, ys,
            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename T3, typename... Args, typename CT, typename R, typename CR>
CR fmap3(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
         const adt::grid<C, T3, D> &zs, Args &&... args) {
  return CR(typename CR::fmap3(), std::forward<F>(f), xs, ys, zs,
            std::forward<Args>(args)...);
}

// fmapStencil

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 0> *, typename CT, typename R,
          typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, Args &&... args) {
  return CR(typename CR::fmapStencilMulti(), std::forward<F>(f),
            std::forward<G>(g), xs, bmask, std::forward<Args>(args)...);
}

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 1> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bp0, Args &&... args) {
  return CR(typename CR::fmapStencilMulti(), std::forward<F>(f),
            std::forward<G>(g), xs, bmask, bm0, bp0,
            std::forward<Args>(args)...);
}

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 2> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, const BCB &bm0, const BCB &bm1,
                    const BCB &bp0, const BCB &bp1, Args &&... args) {
  return CR(typename CR::fmapStencilMulti(), std::forward<F>(f),
            std::forward<G>(g), xs, bmask, bm0, bm1, bp0, bp1,
            std::forward<Args>(args)...);
}

// head, last

template <typename C, typename T, std::size_t D, std::enable_if_t<D == 1> *>
decltype(auto) head(const adt::grid<C, T, D> &xs) {
  return xs.head();
}

template <typename C, typename T, std::size_t D, std::enable_if_t<D == 1> *>
decltype(auto) last(const adt::grid<C, T, D> &xs) {
  return xs.last();
}

// boundary

template <typename C, typename T, std::size_t D, std::enable_if_t<D != 0> *,
          typename CT, typename BC, typename BCT>
BCT boundary(const adt::grid<C, T, D> &xs, std::ptrdiff_t i) {
  return BCT(typename BCT::boundary(), xs, i);
}

// boundaryMap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          std::enable_if_t<D != 0> *, typename CT, typename BC, typename R,
          typename BCR>
BCR boundaryMap(F &&f, const adt::grid<C, T, D> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
          Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename T2, typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
           const adt::grid<C, T2, D> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T, std::enable_if_t<detail::is_grid<C>::value> *,
          typename R, typename CR>
CR munit(T &&x) {
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

template <typename C, typename T, std::size_t D>
decltype(auto) mextract(const adt::grid<C, T, D> &xs) {
  return xs.head();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          std::size_t D, typename... Args, typename C, typename R, typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::grid<A, T, D> &xs,
            Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R, std::enable_if_t<detail::is_grid<C>::value> *,
          typename CR>
CR mzero() {
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

template <typename C, typename T, std::size_t D>
bool mempty(const adt::grid<C, T, D> &xs) {
  return xs.empty();
}

// msize

template <typename C, typename T, std::size_t D>
std::size_t msize(const adt::grid<C, T, D> &xs) {
  return xs.size();
}
}

#define FUN_GRID_IMPL_HPP_DONE
#endif // #ifdef FUN_GRID_IMPL_HPP
#ifndef FUN_GRID_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
