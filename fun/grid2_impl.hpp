#ifndef FUN_GRID2_IMPL_HPP
#define FUN_GRID2_IMPL_HPP

#include "grid2_decl.hpp"

#include <adt/grid2_impl.hpp>

namespace fun {

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid2<C>::value> *, typename R,
          typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  static_assert(fun_traits<C>::rank == 1, "");
  return CR(typename CR::iotaMap(), std::forward<F>(f), inds,
            std::forward<Args>(args)...);
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_grid2<C>::value> *, typename R,
          typename CR>
CR iotaMap(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  return CR(typename CR::iotaMap(), std::forward<F>(f), inds,
            std::forward<Args>(args)...);
}

// fmap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          typename CT, typename R, typename CR>
CR fmap(F &&f, const adt::grid2<C, T, D> &xs, Args &&... args) {
  return CR(typename CR::fmap(), std::forward<F>(f), xs,
            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename... Args, typename CT, typename R, typename CR>
CR fmap2(F &&f, const adt::grid2<C, T, D> &xs, const adt::grid2<C, T2, D> &ys,
         Args &&... args) {
  return CR(typename CR::fmap2(), std::forward<F>(f), xs, ys,
            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename T3, typename... Args, typename CT, typename R, typename CR>
CR fmap3(F &&f, const adt::grid2<C, T, D> &xs, const adt::grid2<C, T2, D> &ys,
         const adt::grid2<C, T3, D> &zs, Args &&... args) {
  return CR(typename CR::fmap3(), std::forward<F>(f), xs, ys, zs,
            std::forward<Args>(args)...);
}

// boundary

template <typename C, typename T, std::size_t D, typename CT>
CT boundary(const adt::grid2<C, T, D> &xs, std::ptrdiff_t f, std::ptrdiff_t d) {
  return CT(typename CT::boundary(), xs, f, d);
}

// fmapStencil

template <typename F, typename G, typename C, typename T, std::size_t D,
          typename B, typename... Args, typename CT, typename R, typename CR>
CR fmapStencil(F &&f, G &&g, const adt::grid2<C, T, D> &xs,
               const std::array<std::array<adt::grid2<C, B, D>, D>, 2> &bss,
               Args &&... args) {
  return CR(typename CR::fmapStencil(), std::forward<F>(f), std::forward<G>(g),
            xs, bss, std::forward<Args>(args)...);
}

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::grid2<C, T, D> &xs,
          Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}
} // namespace fun

#define FUN_GRID2_IMPL_HPP_DONE
#endif // #ifdef FUN_GRID2_IMPL_HPP
#ifndef FUN_GRID2_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
