#ifndef FUN_NESTED_IMPL_HPP
#define FUN_NESTED_IMPL_HPP

#include "nested_decl.hpp"

#include <adt/array.hpp>
#include <adt/dummy.hpp>

#include <adt/nested_impl.hpp>

namespace fun {

// iotaMap

namespace detail {
template <typename A> struct nested_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::irange_t &inds, F &&f,
                  Args &&... args) const {
    return iotaMap<A>(std::forward<F>(f), inds, std::forward<Args>(args)...);
  }
};
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> *, typename R,
          typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CR{iotaMap<P>(
      detail::nested_iotaMap<A>(),
      adt::irange_t(inds.imin(), inds.imax(),
                    inds.empty() ? inds.istep() : inds.imax() - inds.imin()),
      inds, std::forward<F>(f), std::forward<Args>(args)...)};
}

namespace detail {
template <typename A> struct nested_iotaMapMulti : std::tuple<> {
  template <std::size_t D, typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::range_t<D> &inds, F &&f,
                  Args &&... args) const {
    return iotaMapMulti<A>(std::forward<F>(f), inds,
                           std::forward<Args>(args)...);
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> *, typename R,
          typename CR>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  // TODO: Call iotaMapMulti<P> instead?
  return CR{iotaMap<P>(detail::nested_iotaMapMulti<A>(), adt::irange_t(1), inds,
                       std::forward<F>(f), std::forward<Args>(args)...)};
}

// fmap

namespace detail {
struct nested_fmap : std::tuple<> {
  template <typename AT, typename F, typename... Args>
  auto operator()(AT &&xs, F &&f, Args &&... args) const {
    return fmap(std::forward<F>(f), std::forward<AT>(xs),
                std::forward<Args>(args)...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename... Args,
          typename C, typename R, typename CR>
CR fmap(F &&f, const adt::nested<P, A, T> &xss, Args &&... args) {
  return CR{fmap(detail::nested_fmap(), xss.data, std::forward<F>(f),
                 std::forward<Args>(args)...)};
}

namespace detail {
struct nested_fmap2 : std::tuple<> {
  template <typename AT, typename AT2, typename F, typename... Args>
  auto operator()(AT &&xs, AT2 &&ys, F &&f, Args &&... args) const {
    return fmap2(std::forward<F>(f), std::forward<AT>(xs),
                 std::forward<AT2>(ys), std::forward<Args>(args)...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename T2,
          typename... Args, typename C, typename R, typename CR>
CR fmap2(F &&f, const adt::nested<P, A, T> &xss,
         const adt::nested<P, A, T2> &yss, Args &&... args) {
  return CR{fmap2(detail::nested_fmap2(), xss.data, yss.data,
                  std::forward<F>(f), std::forward<Args>(args)...)};
}

namespace detail {
struct nested_fmap3 : std::tuple<> {
  template <typename AT, typename AT2, typename AT3, typename F,
            typename... Args>
  auto operator()(AT &&xs, AT2 &&ys, AT3 &&zs, F &&f, Args &&... args) const {
    return fmap3(std::forward<F>(f), std::forward<AT>(xs),
                 std::forward<AT2>(ys), std::forward<AT3>(zs),
                 std::forward<Args>(args)...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename T2,
          typename T3, typename... Args, typename C, typename R, typename CR>
CR fmap3(F &&f, const adt::nested<P, A, T> &xss,
         const adt::nested<P, A, T2> &yss, const adt::nested<P, A, T3> &zss,
         Args &&... args) {
  return CR{fmap3(detail::nested_fmap3(), xss.data, yss.data, zss.data,
                  std::forward<F>(f), std::forward<Args>(args)...)};
}

// fmapStencil

namespace detail {
struct nested_fmapStencil : std::tuple<> {
  template <typename AT, typename F, typename G, typename BM, typename BP,
            typename... Args>
  auto operator()(AT &&xs, F &&f, G &&g, std::size_t bmask, BM &&bm, BP &&bp,
                  Args &&... args) const {
    return fmapStencil(std::forward<F>(f), std::forward<G>(g),
                       std::forward<AT>(xs), bmask, std::forward<BM>(bm),
                       std::forward<BP>(bp), std::forward<Args>(args)...);
  }
};
}

template <typename F, typename G, typename P, typename A, typename T,
          typename BM, typename BP, typename... Args, typename C, typename B,
          typename R, typename CR>
CR fmapStencil(F &&f, G &&g, const adt::nested<P, A, T> &xss, std::size_t bmask,
               BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return CR{fun::fmap(detail::nested_fmapStencil(), xss.data,
                      std::forward<F>(f), std::forward<G>(g), bmask,
                      std::forward<BM>(bm), std::forward<BP>(bp),
                      std::forward<Args>(args)...)};
}

namespace detail {
template <std::ptrdiff_t> struct nested_fmapStencilMulti;

template <> struct nested_fmapStencilMulti<0> : std::tuple<> {
  template <typename AT, typename F, typename G, typename... Args>
  auto operator()(AT &&xs, F &&f, G &&g, std::size_t bmask,
                  Args &&... args) const {
    return fmapStencilMulti<0>(std::forward<F>(f), std::forward<G>(g),
                               std::forward<AT>(xs), bmask,
                               std::forward<Args>(args)...);
  }
};
}

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 0> *, typename CT,
          typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask, Args &&... args) {
  return CR{fmap(detail::nested_fmapStencilMulti<D>(), xss.data,
                 std::forward<F>(f), std::forward<G>(g), bmask,
                 std::forward<Args>(args)...)};
}

namespace detail {
template <> struct nested_fmapStencilMulti<1> : std::tuple<> {
  template <typename AT, typename ABM0, typename ABP0, typename F, typename G,
            typename... Args>
  auto operator()(AT &&xs, ABM0 &&bm0, ABP0 &&bp0, F &&f, G &&g,
                  std::size_t bmask, Args &&... args) const {
    return fmapStencilMulti<1>(std::forward<F>(f), std::forward<G>(g),
                               std::forward<AT>(xs), bmask,
                               std::forward<ABM0>(bm0), std::forward<ABP0>(bp0),
                               std::forward<Args>(args)...);
  }
};
}

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 1> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    Args &&... args) {
  return CR{fmap3(detail::nested_fmapStencilMulti<D>(), xss.data, bm0.data,
                  bp0.data, std::forward<F>(f), std::forward<G>(g), bmask,
                  std::forward<Args>(args)...)};
}

namespace detail {
template <> struct nested_fmapStencilMulti<2> : std::tuple<> {
  template <typename AT, typename ABM0, typename ABM1, typename ABP0,
            typename ABP1, typename F, typename G, typename... Args>
  auto operator()(AT &&xs, ABM0 &&bm0, ABM1 &&bm1, ABP0 &&bp0, ABP1 &&bp1,
                  F &&f, G &&g, std::size_t bmask, Args &&... args) const {
    return fmapStencilMulti<2>(std::forward<F>(f), std::forward<G>(g),
                               std::forward<AT>(xs), bmask,
                               std::forward<ABM0>(bm0), std::forward<ABM1>(bm1),
                               std::forward<ABP0>(bp0), std::forward<ABP1>(bp1),
                               std::forward<Args>(args)...);
  }
};
}

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 2> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bm1,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    const typename adt::idtype<BCB>::element_type &bp1,
                    Args &&... args) {
  return CR{fmap5(detail::nested_fmapStencilMulti<D>(), xss.data, bm0.data,
                  bm1.data, bp0.data, bp1.data, std::forward<F>(f),
                  std::forward<G>(g), bmask, std::forward<Args>(args)...)};
}

// head, last

namespace detail {
struct nested_head : std::tuple<> {
  template <typename AT> decltype(auto) operator()(AT &&xs) const {
    return head(std::forward<AT>(xs));
  }
};

struct nested_last : std::tuple<> {
  template <typename AT> decltype(auto) operator()(AT &&xs) const {
    return last(std::forward<AT>(xs));
  }
};
}

// Note: The call to fmap is necessary in case we use a proxy; calling
// head or mextract on the proxy would copy the whole data structure
// to the local process, which would be prohibitively expensive.

// Note: Why not use foldMap instead of mextract(fmap)?

template <typename P, typename A, typename T>
decltype(auto) head(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_head(), xss.data));
}

template <typename P, typename A, typename T>
decltype(auto) last(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_last(), xss.data));
}

// boundary

namespace detail {
struct nested_boundary {
  template <typename AT> auto operator()(AT &&xs, std::ptrdiff_t i) const {
    return boundary(std::forward<AT>(xs), i);
  }
};
}

template <typename P, typename A, typename T, typename CT, typename BC,
          typename BCT>
BCT boundary(const adt::nested<P, A, T> &xs, std::ptrdiff_t i) {
  return BCT{fmap(detail::nested_boundary(), xs.data, i)};
}

// boudaryMap

template <typename F, typename P, typename A, typename T, typename... Args,
          typename CT, typename BC, typename R, typename BCR>
BCR boundaryMap(F &&f, const adt::nested<P, A, T> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// indexing

namespace detail {
struct nested_getIndex : std::tuple<> {
  template <typename AT>
  decltype(auto) operator()(AT &&xs, std::ptrdiff_t i) const {
    return std::forward<AT>(xs)[i];
  }
};
}

template <typename P, typename A, typename T>
decltype(auto) getIndex(const adt::nested<P, A, T> &xs, std::ptrdiff_t i) {
  return mextract(fmap(detail::nested_getIndex(), xs.data, i));
}

template <typename P, typename A, typename T>
adt::nested<P, A, T> accumulator<adt::nested<P, A, T>>::finalize() {
  return {munit<P>(data.finalize())};
}

// foldMap

namespace detail {
struct nested_foldMap : std::tuple<> {
  template <typename AT, typename F, typename Op, typename Z, typename... Args>
  auto operator()(AT &&xs, F &&f, Op &&op, Z &&z, Args &&... args) const {
    return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                   std::forward<AT>(xs), std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap(detail::nested_foldMap(), op, z, xss.data, std::forward<F>(f),
                 op, z, std::forward<Args>(args)...);
}

namespace detail {
struct nested_foldMap2 : std::tuple<> {
  template <typename AT, typename AT2, typename F, typename Op, typename Z,
            typename... Args>
  auto operator()(const AT &xs, const AT2 &ys, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return foldMap2(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), xs, ys, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename T2, typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
           const adt::nested<P, A, T2> &yss, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap2(detail::nested_foldMap2(), op, z, xss.data, yss.data,
                  std::forward<F>(f), op, z, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CT>
CT munit(T &&x) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CT{munit<P>(munit<A>(std::forward<T>(x)))};
}

// mjoin

namespace detail {
struct nested_mextract : std::tuple<> {
  // TODO: Why can't this be decltype(auto)? I assume a std::decay_t is missing
  // somewhere.
  template <typename NPAT> auto operator()(const NPAT &xss) const {
    return mextract(xss.data);
  }
};
struct nested_mjoin : std::tuple<> {
  template <typename ANPAT> auto operator()(ANPAT &&xsss) const {
    return mjoin(fmap(nested_mextract(), std::forward<ANPAT>(xsss)));
  }
};
}

template <typename P, typename A, typename T, typename CT>
CT mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss) {
  return CT{fmap(detail::nested_mjoin(), xssss.data)};
}

// mbind

template <typename F, typename P, typename A, typename T, typename... Args,
          typename CR>
CR mbind(F &&f, const adt::nested<P, A, T> &xss, Args &&... args) {
  static_assert(detail::is_nested<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xss, std::forward<Args>(args)...));
}

// mextract

template <typename P, typename A, typename T>
decltype(auto) mextract(const adt::nested<P, A, T> &xss) {
  return mextract(mextract(xss.data));
}

// mfoldMap

namespace detail {
struct nested_mfoldMap : std::tuple<> {
  template <typename AT, typename F, typename Op, typename Z, typename... Args>
  auto operator()(AT &&xs, F &&f, Op &&op, Z &&z, Args &&... args) const {
    return mfoldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<AT>(xs),
                    std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args, typename C, typename R, typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
            Args &&... args) {
  return CR{fmap(detail::nested_mfoldMap(), xss.data, std::forward<F>(f),
                 std::forward<Op>(op), std::forward<Z>(z),
                 std::forward<Args>(args)...)};
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CR>
CR mzero() {
  typedef typename C::pointer_dummy P;
  typedef typename C::template array_constructor<R> AR;
  return CR{mzero<P, AR>()};
}

// mplus

template <typename P, typename A, typename T, typename... Ts, typename CT>
CT mplus(const adt::nested<P, A, T> &xss,
         const adt::nested<P, A, Ts> &... yss) {
  typedef typename CT::template array_constructor<T> AT;
  return CT{munit<P, AT>(
      mplus(mempty(xss.data) ? mzero<A, T>() : mextract(xss.data),
            mempty(yss.data) ? mzero<A, T>() : mextract(yss.data)...))};
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CT>
CT msome(T &&x, Ts &&... ys) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CT{munit<P>(msome<A>(std::forward<T>(x), std::forward<Ts>(ys)...))};
}

// mempty

template <typename P, typename A, typename T>
bool mempty(const adt::nested<P, A, T> &xss) {
  return mempty(xss.data);
}

// msize

template <typename P, typename A, typename T>
std::size_t msize(const adt::nested<P, A, T> &xss) {
  typedef typename adt::nested<P, A, T>::template array_constructor<T> AT;
  return mempty(xss.data) ? 0 : foldMap((std::size_t (*)(const AT &))msize,
                                        std::plus<std::size_t>(), 0, xss.data);
}
}

#define FUN_NESTED_IMPL_HPP_DONE
#endif // #ifdef FUN_NESTED_IMPL_HPP
#ifndef FUN_NESTED_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
