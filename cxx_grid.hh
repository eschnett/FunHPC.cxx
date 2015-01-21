#ifndef CXX_GRID_HH
#define CXX_GRID_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_shape.hh"

#include <cereal/access.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

namespace cxx {

// Cartesian grid

// Safe helper for foldable
template <typename F, typename Op, std::ptrdiff_t D, typename T, typename T1,
          typename... As>
struct grid_foldMapSafe {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d >= 0 && d < D), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const std::vector<T1> &xs,
          const index<D> &pos, const As &... as) {
    T r(z);
    for (std::ptrdiff_t a = rr.imin()[d]; a < rr.imax()[d]; a += rr.istep()[d])
      r = cxx::invoke(op, std::move(r), foldMap<d - 1>(f, op, z, rr, xr, xs,
                                                       pos.set(d, a), as...));
    return r;
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const std::vector<T1> &xs,
          const index<D> &pos, const As &... as) {
    return cxx::invoke(f, xs.at(xr.linear(pos)), as...);
  }
};

// Helper for foldable
template <typename F, typename Op, std::ptrdiff_t D, typename T, typename T1,
          typename... As>
struct grid_foldMap {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t xstr = xr.strides()[d];
    assert(all_of(rr.imin() >= xr.imin() && rr.imax() <= xr.imax()));
    assert(all_of(rr.istep() == xr.istep()));
    auto ilast = rr.imin().set(d, (rr.imax() - rr.istep())[d]);
    assert(xr.linear(rr.imin()) + (nelts - 1) * xstr == xr.linear(ilast));
    // T r(z);
    // for (std::ptrdiff_t a = 0; a < nelts; ++a)
    //   r = cxx::invoke(op, std::move(r),
    //                   foldMap<d - 1>(f, op, z, rr, xr, xs + a * xstr,
    //                   as...));
    std::vector<rpc::future<T> > rs(nelts);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      rs[a] =
          rpc::async(foldMap<d - 1>, f, op, z, rr, xr, xs + a * xstr, as...);
    T r(z);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r), rs[a].get());
    return r;
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    assert(rr.strides()[d] == 1);
    assert(xr.strides()[d] == 1);
    assert(all_of(rr.imin() >= xr.imin() && rr.imax() <= xr.imax()));
    assert(all_of(rr.istep() == xr.istep()));
    auto ilast = rr.imin().set(d, (rr.imax() - rr.istep())[d]);
    assert(xr.linear(rr.imin()) + (nelts - 1) == xr.linear(ilast));
#pragma omp declare reduction(op : T : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
//     T r(z);
// #pragma omp simd reduction(op : r)
//     for (std::ptrdiff_t a = 0; a < nelts; ++a)
//       r = cxx::invoke(op, std::move(r),
//                       foldMap<d - 1>(f, op, z, rr, xr, xs + a, as...));
#warning                                                                       \
    "TODO: this is not efficient; put parallelisation into tree, via (fmap2 fold)"
    std::vector<rpc::future<T> > rs(nelts);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      rs[a] = rpc::async(foldMap<d - 1>, f, op, z, rr, xr, xs + a, as...);
    T r(z);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r), rs[a].get());
    return r;
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    return cxx::invoke(f, *xs, as...);
  }
};

template <typename F, typename Op, std::ptrdiff_t D, typename T, typename T1,
          typename T2, typename... As>
struct grid_foldMap2 {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t xstr = xr.strides()[d];
    std::ptrdiff_t ystr = yr.strides()[d];
    T r(z);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r),
                      foldMap2<d - 1>(f, op, z, rr, xr, xs + a * xstr, yr,
                                      ys + a * ystr, as...));
    return r;
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    assert(rr.strides()[d] == 1);
    assert(xr.strides()[d] == 1);
    assert(yr.strides()[d] == 1);
#pragma omp declare reduction(op : T : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
    T r(z);
#pragma omp simd reduction(op : r)
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(
          op, std::move(r),
          foldMap2<d - 1>(f, op, z, rr, xr, xs + a, yr, ys + a, as...));
    return r;
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    return cxx::invoke(f, *xs, *ys, as...);
  }
};

// Safe helper for functor
template <typename F, std::ptrdiff_t D, typename T, typename T1, typename... As>
struct grid_fmapSafe {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d >= 0 && d < D), void>::type
  fmap(const F &f, const grid_region<D> &rr, std::vector<T> &rs,
       const grid_region<D> &xr, const std::vector<T1> &xs, const index<D> &pos,
       const As &... as) {
    for (std::ptrdiff_t a = rr.imin()[d]; a < rr.imax()[d]; a += rr.istep()[d])
      fmap<d - 1>(f, rr, rs, xr, xs, pos.set(d, a), as...);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  fmap(const F &f, const grid_region<D> &rr, std::vector<T> &rs,
       const grid_region<D> &xr, const std::vector<T1> &xs, const index<D> &pos,
       const As &... as) {
    rs.at(rr.linear(pos)) = cxx::invoke(f, xs.at(xr.linear(pos)), as...);
  }
};

// Helper for functor
template <typename F, std::ptrdiff_t D, typename T, typename T1, typename... As>
struct grid_fmap {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.strides()[d];
    std::ptrdiff_t xstr = xr.strides()[d];
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap<d - 1>(f, rr, rs + a * rstr, xr, xs + a * xstr, as...);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    assert(rr.strides()[d] == 1);
    assert(xr.strides()[d] == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap<d - 1>(f, rr, rs + a, xr, xs + a, as...);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    *rs = cxx::invoke(f, *xs, as...);
  }
};

template <typename F, std::ptrdiff_t D, typename T, typename T1, typename T2,
          typename... As>
struct grid_fmap2 {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.strides()[d];
    std::ptrdiff_t xstr = xr.strides()[d];
    std::ptrdiff_t ystr = yr.strides()[d];
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap2<d - 1>(f, rr, rs + a * rstr, xr, xs + a * xstr, yr, ys + a * ystr,
                   as...);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    assert(rr.strides()[d] == 1);
    assert(xr.strides()[d] == 1);
    assert(yr.strides()[d] == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap2<d - 1>(f, rr, rs + a, xr, xs + a, yr, ys + a, as...);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    *rs = cxx::invoke(f, *xs, *ys, as...);
  }
};

template <typename F, typename G, std::ptrdiff_t D, typename T, typename T1,
          typename B, typename... As>
struct grid_stencil_fmap {
  static constexpr std::size_t bit(std::size_t i) {
    return std::size_t(1) << i;
  }
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t dir, std::size_t isouters = 0>
  static typename std::enable_if<(dir > 0 && dir < D), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[dir];
    std::ptrdiff_t rstr = rr.strides()[dir];
    std::ptrdiff_t xstr = xr.strides()[dir];
    if (nelts == 1) {
      // both boundaries
      constexpr std::size_t newisouters =
          isouters | (bit(2 * dir + 0) | bit(2 * dir + 1));
      std::ptrdiff_t a = 0;
      auto newbss = bss;
      for (std::ptrdiff_t f = 0; f < 2; ++f)
        for (std::ptrdiff_t d = 0; d < D; ++d)
          if (d != dir)
            newbss(d, f) += a * brs(d, f).strides()[dir];
      stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                         xs + a * xstr, brs, newbss, as...);
    } else if (nelts > 1) {
      // lower boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 0);
        std::ptrdiff_t a = 0;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).strides()[dir];
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
      // interior
      for (std::ptrdiff_t a = 1; a < nelts - 1; ++a) {
        constexpr std::size_t newisouters = isouters;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).strides()[dir];
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
      // upper boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 1);
        std::ptrdiff_t a = nelts - 1;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).strides()[dir];
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
    }
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t dir, std::size_t isouters = 0>
  static typename std::enable_if<(dir == 0 && dir < D), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[dir];
    assert(rr.strides()[dir] == 1);
    assert(xr.strides()[dir] == 1);
    for (std::ptrdiff_t f = 0; f < 2; ++f)
      for (std::ptrdiff_t d = 0; d < D; ++d)
        assert(brs(d, f).strides()[dir] == 1);
    if (nelts == 1) {
      // both boundaries
      constexpr std::size_t newisouters =
          isouters | (bit(2 * dir + 0) | bit(2 * dir + 1));
      std::ptrdiff_t a = 0;
      auto newbss = bss;
      for (std::ptrdiff_t f = 0; f < 2; ++f)
        for (std::ptrdiff_t d = 0; d < D; ++d)
          if (d != dir)
            newbss(d, f) += a;
      stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                         newbss, as...);
    } else if (nelts > 1) {
      // lower boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 0);
        std::ptrdiff_t a = 0;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
// interior
#pragma omp simd
      for (std::ptrdiff_t a = 1; a < nelts - 1; ++a) {
        constexpr std::size_t newisouters = isouters;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
      // upper boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 1);
        std::ptrdiff_t a = nelts - 1;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
    }
  }
  // Terminating case for single elements
  template <std::ptrdiff_t dir, std::size_t isouters>
  static typename std::enable_if<(dir == -1), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    boundaries<B, D> newbs;
    for (std::ptrdiff_t f = 0; f < 2; ++f) {
      for (std::ptrdiff_t d = 0; d < D; ++d) {
        bool isouter = isouters & bit(2 * d + f);
        auto off = (!f ? -1 : +1) * xr.strides()[d];
        newbs(d, f) = isouter ? *bss(d, f) : cxx::invoke(g, xs[off], d, !f);
      }
    }
    *rs = cxx::invoke(f, *xs, newbs, as...);
  }
};

// Safe helper for iota
template <typename F, std::ptrdiff_t D, typename T, typename... As>
struct grid_iotaSafe {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d >= 0 && d < D), void>::type
  iota(const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       std::vector<T> &rs, const index<D> &pos, const As &... as) {
    for (std::ptrdiff_t a = rr.imin()[d]; a < rr.imax()[d];
         a += rr.istep()[d]) {
      iota<d - 1>(f, gr, rr, rs, pos.set(d, a), as...);
    }
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  iota(const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       std::vector<T> &rs, const index<D> &pos, const As &... as) {
    assert(all_of(pos >= rr.imin() && pos < rr.imax()));
    assert(all_of(pos >= gr.imin() && pos < gr.imax()));
    rs.at(rr.linear(pos)) = cxx::invoke(f, gr, pos, as...);
  }
};

// Helper for iota
template <typename F, std::ptrdiff_t D, typename T, typename... As>
struct grid_iota {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  iota(const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       T *restrict rs, const index<D> &imin, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.strides()[d];
    index<D> idir = rr.istep() * index<D>::dir(d);
    for (std::ptrdiff_t a = 0; a < nelts; ++a) {
      index<D> i = imin + a * idir;
      iota<d - 1>(f, gr, rr, rs + a * rstr, i, as...);
    }
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  iota(const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       T *restrict rs, index<D> imin, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    index<D> idir = rr.istep() * index<D>::dir(d);
    assert(rr.strides()[d] == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a) {
      index<D> i = imin + a * idir;
      iota<d - 1>(f, gr, rr, rs + a, i, as...);
    }
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  iota(const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       T *restrict rs, const index<D> &i, const As &... as) {
    *rs = cxx::invoke(f, gr, i, as...);
  }
};

template <typename T, std::ptrdiff_t D> class grid {
  template <typename T1, std::ptrdiff_t D1> friend class grid;

  // Data may be shared with other grids
  // TODO: Introdce "grid" and "gridview", or "gridstorage" and "grid"
  std::shared_ptr<std::vector<T> > data_;
  grid_region<D> layout_; // data memory layout
  grid_region<D> region_; // our grid_region

  grid(const grid_region<D> &region)
      : data_(std::make_shared<std::vector<T> >(region.size())),
        layout_(region), region_(region) {}

  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const {
    // Are we referring to all of the data?
    // TODO: handle padding
    if (region_ == layout_) {
      // Yes: save everything
      ar(data_, layout_, region_);
    } else {
      // No: create a copy of the respective sub-grid_region, and save it
      grid(copy(), *this).save(ar);
    }
  }
  template <typename Archive> void load(Archive &ar) {
    ar(data_, layout_, region_);
  }

public:
  bool invariant() const {
    return bool(data_) && layout_.size() == data_->size() &&
           region_.is_subregion_of(layout_);
  }
  grid_region<D> region() const { return region_; }
  bool empty() const { return region_.empty(); }
  std::ptrdiff_t size() const { return region_.size(); }
  index<D> shape() const { return region_.shape(); }
  //
  // TODO: hide this function
  grid() : grid(grid_region<D>()) {}
  struct copy : std::tuple<> {};
  grid(copy, const grid &g)
      : grid(fmap(), [](const T &x) { return x; }, region_, g) {}
  struct subregion : std::tuple<> {};
  grid(subregion, const grid &xs, const grid_region<D> &region)
      : data_(xs.data_), layout_(xs.layout_), region_(region) {
    assert(region.is_subregion_of(xs.region_));
  }
  struct boundary : std::tuple<> {};
  grid(boundary, const grid &xs, std::ptrdiff_t dir, bool face)
      : grid(subregion(), xs, xs.region_.boundary(dir, face)) {}
  // foldable
  const T &head() const {
    // return (*data_).at(layout_.linear(region_.imin()));
    return (*data_)[layout_.linear(region_.imin())];
  }
  const T &last() const {
    // return (*data_).at(layout_.linear(region_.imax() - region_.istep()));
    return (*data_)[layout_.linear(region_.imax() - region_.istep())];
  }
  template <typename F, typename Op, typename R, typename... As>
  auto foldMap(const F &f, const Op &op, const R &z, const As &... as) const {
    static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    return grid_foldMap<std::decay_t<F>, std::decay_t<Op>, D, R, T,
                        As...>::template foldMap<D - 1>(f, op, z, region_,
                                                        layout_,
                                                        data_->data() +
                                                            layout_.linear(
                                                                region_.imin()),
                                                        as...);
  }
  template <typename F, typename Op, typename R, typename T1, typename... As>
  auto foldMap2(const F &f, const Op &op, const R &z, const grid<T1, D> &ys,
                const As &... as) const {
    static_assert(std::is_same<cxx::invoke_of_t<F, T, T1, As...>, R>::value,
                  "");
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    assert(region_.is_subregion_of(ys.region_));
    return grid_foldMap2<F, Op, D, R, T, T1, As...>::template foldMap2<D - 1>(
        f, op, z, region_, layout_,
        data_->data() + layout_.linear(region_.imin()), ys.layout_,
        ys.data_->data() + ys.layout_.linear(ys.region_.imin()), as...);
  }
  // functor
  struct fmap : std::tuple<> {};
  template <typename F, typename T1, typename... As>
  grid(fmap, const F &f, const grid_region<D> &rr, const grid<T1, D> &xs,
       const As &... as)
      : grid(rr) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, As...>, T>::value, "");
    assert(rr.is_subregion_of(xs.region_));
    grid_fmap<F, D, T, T1, As...>::template fmap<D - 1>(
        f, layout_, data_->data(), xs.layout_,
        xs.data_->data() + xs.layout_.linear(xs.region_.imin()), as...);
    // grid_fmapSafe<F, D, T, T1, As...>::template fmap<D - 1>(
    //     f, layout_, *data_, xs.layout_, *xs.data_, index<D>::zero(), as...);
  }
  struct fmap2 : std::tuple<> {};
  template <typename F, typename T1, typename T2, typename... As>
  grid(fmap2, const F &f, const grid_region<D> &rr, const grid<T1, D> &xs,
       const grid<T2, D> &ys, const As &... as)
      : grid(rr) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, As...>, T>::value,
                  "");
    assert(rr.is_subregion_of(xs.region_));
    assert(rr.is_subregion_of(ys.region_));
    grid_fmap2<F, D, T, T1, T2, As...>::template fmap2<D - 1>(
        f, layout_, data_->data(), xs.layout_,
        xs.data_->data() + xs.layout_.linear(xs.region_.imin()), ys.layout_,
        ys.data_->data() + ys.layout_.linear(ys.region_.imin()), as...);
  }
  struct stencil_fmap : std::tuple<> {};
  template <typename F, typename G, typename T1, typename B, typename... As>
  grid(stencil_fmap, const F &f, const G &g, const grid_region<D> &rr,
       const grid<T1, D> &xs, const boundaries<grid<B, D>, D> &bs,
       const As &... as)
      : grid(rr) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, boundaries<B, D>, As...>,
                               T>::value,
                  "");
    static_assert(
        std::is_same<cxx::invoke_of_t<G, T1, std::ptrdiff_t, bool>, B>::value,
        "");
    assert(rr.is_subregion_of(xs.region_));
    boundaries<grid_region<D>, D> brs;
    boundaries<const B * restrict, D> bss;
    for (std::ptrdiff_t f = 0; f < 2; ++f) {
      for (std::ptrdiff_t d = 0; d < D; ++d) {
        assert(all_of(bs(d, f).region_.shape() ==
                      xs.region_.boundary(d, f).shape()));
        // TODO: also check that indices match
        // TODO: introduce function to shift a domain, so that the
        // outer boundaries can be created from the boundary of the
        // interior via shifting
        // assert(all_of(bs(d, f).region_.imin() ==
        //               xs.region_.boundary(d, f).imin() +
        //                   std::ptrdiff_t(!f ? -1 : +1) * index<D>::dir(d)));
        brs(d, f) = bs(d, f).region_;
        bss(d, f) = bs(d, f).data_->data();
      }
    }
    grid_stencil_fmap<F, G, D, T, T1, B, As...>::template stencil_fmap<D - 1>(
        f, g, layout_, data_->data(), xs.layout_, xs.data_->data(), brs, bss,
        as...);
  }
  // iota
  struct iota : std::tuple<> {};
  template <typename F, typename... As>
  grid(iota, const F &f, const grid_region<D> &gr, const grid_region<D> &rr,
       const As &... as)
      : grid(rr) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
                     T>::value,
        "");
    grid_iota<F, D, T, As...>::template iota<D - 1>(
        f, gr, layout_, data_->data(), region_.imin(), as...);
    // grid_iotaSafe<F, D, T, As...>::template iota<D - 1>(
    //     f, gr, layout_, *data_, index<D>::zero(), as...);
  }
};

// kinds

template <typename T, std::ptrdiff_t D> struct kinds<grid<T, D> > {
  typedef T value_type;
  template <typename U> using constructor = grid<U, D>;
};
template <typename T> struct is_grid : std::false_type {};
template <typename T, std::ptrdiff_t D>
struct is_grid<grid<T, D> > : std::true_type {};

// foldable

template <typename Op, typename T, std::ptrdiff_t D, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, T, T, As...>::type, T>::value,
    T>::type
fold(const Op &op, const T &z, const grid<T, D> &xs, const As &... as) {
  return xs.foldMap([](const T &x) { return x; }, op, z, as...);
}

template <typename T, std::ptrdiff_t D> const T &head(const grid<T, D> &xs) {
  return xs.head();
}
template <typename T, std::ptrdiff_t D> const T &last(const grid<T, D> &xs) {
  return xs.last();
}

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const grid<T, D> &xs,
        const As &... as) {
  return xs.foldMap(f, op, z, as...);
}

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename T2, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, const R &z, const grid<T, D> &xs,
         const grid<T2, D> &ys, const As &... as) {
  return xs.foldMap2(f, op, z, ys, as...);
}

// functor

template <typename F, typename T, std::ptrdiff_t D, typename... As,
          typename CT = grid<T, D>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
auto fmap(const F &f, const grid<T, D> &xs, const As &... as) {
  return C<R>(typename C<R>::fmap(), f, xs.region(), xs, as...);
}

template <typename F, typename T, std::ptrdiff_t D, typename T2, typename... As,
          typename CT = grid<T, D>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
auto fmap2(const F &f, const grid<T, D> &xs, const grid<T2, D> &ys,
           const As &... as) {
  return C<R>(typename C<R>::fmap2(), f, xs.region(), xs, ys, as...);
}

#if 0
template <
    typename F, typename T, std::ptrdiff_t D, typename T2, typename... As,
    typename CT = grid<T, D>,
    template <typename> class C = cxx::kinds<CT>::template constructor,
    typename R = typename cxx::invoke_of<F, T, boundaries<T2, D>, As...>::type>
auto fmap_boundaries(const F &f, const grid<T, D> &xs,
                     const boundaries<grid<T2, D>, D> &yss, const As &... as) {
  static_assert(all_of([&xs](const auto &xs, const auto &ys) {
                         return ys.region() == xs.region();
                       },
                       yss),
                "");
  return C<R>(typename C<R>::fmap_boundaries(), f, xs.region(), xs, yss, as...);
}
#endif

template <typename F, typename T, std::ptrdiff_t D, typename... As>
auto boundary(const F &f, const grid<T, D> &xs, std::ptrdiff_t dir, bool face,
              const As &... as) {
  typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;
  return cxx::fmap(f,
                   grid<T, D>(typename grid<T, D>::boundary(), xs, dir, face),
                   dir, face, as...);
}

template <typename F, typename G, typename T, std::ptrdiff_t D, typename B,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const grid<T, D> &xs,
                  const boundaries<grid<B, D>, D> &bs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
  static_assert(
      std::is_same<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>, B>::value, "");
  return grid<R, D>(typename grid<R, D>::stencil_fmap(), f, g, xs.region(), xs,
                    bs, as...);
}

template <typename F, typename G, typename T, std::ptrdiff_t D, typename B,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const grid<T, D> &xs, const B &bm,
                  const B &bp, const As &... as) {
  static_assert(D == 1, "");
  static_assert(std::is_same<cxx::invoke_of_t<G, T, bool>, B>::value, "");
  boundaries<B, D> bnds(vect<B, D>::set1(bm), vect<B, D>::set1(bp));
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  return stencil_fmap(
      [&f](const T &x, const boundaries<B, D> &bs, const As &... as) {
        return cxx::invoke(f, x, bs(0, false), bs(0, true), as...);
      },
      [&g](const T &x, std::ptrdiff_t dir,
           bool face) { return cxx::invoke(g, x, face); },
      bnds, as...);
}

// iota

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As, typename T = typename cxx::invoke_of<
                              F, grid_region<D>, index<D>, As...>::type,
          std::enable_if_t<cxx::is_grid<C<T> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  return C<T>(typename C<T>::iota(), f, global_range, range, as...);
}

// output

template <typename T, std::ptrdiff_t D, typename CT = grid<T, D>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
std::ostream &operator<<(std::ostream &os, const grid<T, D> &g) {
  // TODO: This is not efficient
  auto is =
      cxx::iota<C>([](const grid_region<D> &, const index<D> &i) { return i; },
                   g.region(), g.region());
  auto ss = fmap2([](const index<D> &i, const T &x) {
                    std::ostringstream os;
                    os << "  " << i << ": " << x << "\n";
                    return os.str();
                  },
                  is, g);
  auto s =
      fold([](const std::string &xs, const std::string &ys) { return xs + ys; },
           std::string(), ss);
  return os << s;
}

// monad

template <template <typename> class C, typename T,
          std::enable_if_t<cxx::is_grid<C<T> >::value> * = nullptr>
auto mzero() {
  typedef decltype(C<T>().region()) region_t;
  return iota<C>([](const auto &, const auto &) { return T(); }, region_t(),
                 region_t());
}
}

#define CXX_GRID_HH_DONE
#else
#ifndef CXX_GRID_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_GRID_HH
