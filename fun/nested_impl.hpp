#ifndef FUN_NESTED_IMPL_HPP
#define FUN_NESTED_IMPL_HPP

#include "nested_decl.hpp"

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/cstdlib.hpp>

#include <adt/nested_impl.hpp>

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace fun {

// iotaMap

namespace detail {

#if 0

template <typename Policy>
std::ptrdiff_t nested_calc_outer_size(const Policy &policy,
                                      const adt::irange_t &inds) {
  bool unlimited_inner = policy.max_inner_size() == std::size_t(-1);
  std::ptrdiff_t min_inner = policy.min_inner_size();
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : policy.max_inner_size();
  bool unlimited_outer = policy.max_outer_size() == std::size_t(-1);
  std::ptrdiff_t min_outer = policy.min_outer_size();
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : policy.max_outer_size();

  std::ptrdiff_t size = inds.size();
  std::ptrdiff_t isize, osize;
  if (size == 0) {
    // empty range, decide whether to create empty or unit outer container
    isize = 0;
    osize = min_outer;
  } else if (unlimited_inner || size <= max_inner) {
    // need only unit outer container, do so
    isize = size;
    osize = 1;
  } else {
    // create outer container with multiple elements, each with a maximal inner
    // container -- we assume this is most efficient
    isize = max_inner;
    osize = cxx::div_ceil(size, max_inner).quot;
  }
  assert(isize * osize >= size);
  if (isize == 0 || osize == 0) {
    assert(size == 0);
  } else {
    // assert((isize - 1) * osize < size);
    assert(isize * (osize - 1) < size);
  }
  assert(isize >= min_inner);
  assert(unlimited_inner || isize <= max_inner);
  assert(osize >= min_outer);
  assert(unlimited_outer || osize <= max_outer);
  return osize;
}

template <typename Policy>
adt::irange_t nested_calc_outer_inds(const Policy &policy,
                                     const adt::irange_t &inds) {
  bool unlimited_inner = policy.max_inner_size() == std::size_t(-1);
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : policy.max_inner_size();
  bool unlimited_outer = policy.max_outer_size() == std::size_t(-1);
  std::ptrdiff_t min_outer = policy.min_outer_size();
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : policy.max_outer_size();

  std::ptrdiff_t osize = nested_calc_outer_size(policy, inds);
  std::ptrdiff_t omin = inds.imin();
  std::ptrdiff_t omax = osize == 0 ? inds.imin() : inds.imax();
  std::ptrdiff_t ostep =
      unlimited_inner ? omax - omin : inds.istep() * max_inner;
  adt::irange_t oinds(omin, omax, ostep);
  assert(oinds.size() >= min_outer);
  assert(unlimited_outer || oinds.size() <= max_outer);
  assert(oinds.size() == osize);
  return oinds;
}

template <typename Policy>
adt::irange_t
nested_calc_inner_inds(const Policy &policy, const adt::irange_t &inds,
                       const adt::irange_t &oinds, std::ptrdiff_t i) {
  bool unlimited_inner = policy.max_inner_size() == std::size_t(-1);
  std::ptrdiff_t min_inner = policy.min_inner_size();
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : policy.max_inner_size();
  std::ptrdiff_t min_outer = policy.min_outer_size();

  std::ptrdiff_t imin = i;
  std::ptrdiff_t imax = std::min(imin + oinds.istep(), inds.imax());
  std::ptrdiff_t istep = inds.istep();
  adt::irange_t iinds(imin, imax, istep);
  assert(iinds.size() >= min_inner);
  assert(unlimited_inner || iinds.size() <= max_inner);
  if (min_outer == 0)
    assert(!iinds.empty());
  return iinds;
}

#endif

template <typename P, typename A>
std::ptrdiff_t nested_calc_outer_size(const adt::irange_t &inds) {
  bool unlimited_outer = fun_traits<P>::max_size == std::size_t(-1);
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : fun_traits<P>::max_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t min_inner = fun_traits<A>::min_size;
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  std::ptrdiff_t size = inds.size();
  std::ptrdiff_t isize, osize;
  if (size == 0) {
    // empty range, decide whether to create empty or unit outer container
    isize = 0;
    osize = max_outer;
  } else if (unlimited_inner || size <= max_inner) {
    // need only unit outer container, do so
    isize = size;
    osize = 1;
  } else {
    // create outer container with multiple elements, each with a maximal inner
    // container -- we assume this is most efficient
    isize = max_inner;
    osize = cxx::div_ceil(size, max_inner).quot;
  }
  assert(isize * osize >= size);
  if (isize == 0 || osize == 0) {
    assert(size == 0);
  } else {
    // assert((isize - 1) * osize < size);
    assert(isize * (osize - 1) < size);
  }
  assert(isize >= min_inner);
  assert(unlimited_inner || isize <= max_inner);
  assert(osize >= min_outer);
  assert(unlimited_outer || osize <= max_outer);
  return osize;
}

template <typename P, typename A>
adt::irange_t nested_calc_outer_inds(const adt::irange_t &inds) {
  bool unlimited_outer = fun_traits<P>::max_size == std::size_t(-1);
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : fun_traits<P>::max_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  std::ptrdiff_t osize = nested_calc_outer_size<P, A>(inds);
  std::ptrdiff_t omin = inds.imin();
  std::ptrdiff_t omax = osize == 0 ? inds.imin() : inds.imax();
  std::ptrdiff_t ostep =
      unlimited_inner ? omax - omin : inds.istep() * max_inner;
  adt::irange_t oinds(omin, omax, ostep);
  assert(oinds.size() >= min_outer);
  assert(unlimited_outer || oinds.size() <= max_outer);
  assert(oinds.size() == osize);
  return oinds;
}

template <typename P, typename A>
adt::irange_t nested_calc_inner_inds(const adt::irange_t &inds,
                                     const adt::irange_t &oinds,
                                     std::ptrdiff_t i) {
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t min_inner = fun_traits<A>::min_size;
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  std::ptrdiff_t imin = i;
  std::ptrdiff_t imax = std::min(imin + oinds.istep(), inds.imax());
  std::ptrdiff_t istep = inds.istep();
  adt::irange_t iinds(imin, imax, istep);
  assert(iinds.size() >= min_inner);
  assert(unlimited_inner || iinds.size() <= max_inner);
  if (min_outer == 0)
    assert(!iinds.empty());
  return iinds;
}

template <typename CR> struct nested_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::irange_t &inds,
                  const adt::irange_t &oinds, F &&f, Args &&... args) const {
    typedef typename CR::pointer_dummy P;
    typedef typename CR::array_dummy A;
    auto iinds = nested_calc_inner_inds<P, A>(inds, oinds, i);
    return iotaMap<A>(std::forward<F>(f), iinds, std::forward<Args>(args)...);
  }
};
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> *, typename R,
          typename CR>
CR iotaMap(const typename CR::policy_type &policy, F &&f,
           const adt::irange_t &inds, Args &&... args) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  auto oinds = detail::nested_calc_outer_inds<P, A>(inds);
  return CR{iotaMap<P>(detail::nested_iotaMap<CR>(), oinds, inds, oinds,
                       std::forward<F>(f), std::forward<Args>(args)...),
            policy};
}

namespace detail {
template <typename P, typename A, std::size_t D>
adt::index_t<D> nested_calc_outer_shape(const adt::range_t<D> &inds) {
  bool unlimited_outer = fun_traits<P>::max_size == std::size_t(-1);
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : fun_traits<P>::max_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t min_inner = fun_traits<A>::min_size;
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  std::size_t size = inds.size();
  adt::index_t<D> ishape, oshape;
  if (size == 0) {
    // empty range, decide whether to create empty or unit outer container
    ishape = adt::range_t<D>::zero();
    oshape = min_outer == 0 ? adt::range_t<D>::zero() : adt::range_t<D>::one();
  } else if (unlimited_inner || size <= max_inner) {
    // need only unit outer container, do so
    ishape = inds.shape();
    oshape = adt::range_t<D>::one();
  } else {
    // create outer container with multiple elements, each with a maximal inner
    // container -- we assume this is most efficient
    std::ptrdiff_t ilinsize = std::floor(std::pow(double(max_inner), 1.0 / D));
    if (cxx::ipow(ilinsize + 1, D) <= max_inner)
      ++ilinsize;
    ishape = adt::set<adt::index_t<D>>(ilinsize);
    // for (std::size_t d = 0; d < D; ++d) {
    //   for (;;) {
    //     auto trial_ishape = adt::update(ishape, d, ishape[d] + 1);
    //     if (adt::prod(trial_ishape) > max_inner)
    //       break;
    //     ishape = trial_ishape;
    //   }
    // }
    ishape = adt::min(inds.shape(), ishape);
    oshape = adt::div_quot(adt::div_ceil(inds.shape(), ishape));
  }
  std::ptrdiff_t isize = adt::prod(ishape);
  std::ptrdiff_t osize = adt::prod(oshape);
  assert(isize * osize >= size);
  if (isize == 0 || osize == 0) {
    assert(size == 0);
  } else {
    // assert((isize - 1) * osize < size);
    for (std::size_t d = 0; d < D; ++d)
      assert(isize * adt::prod(adt::update(oshape, d, oshape[d] + 1)) > size);
  }
  assert(isize >= min_inner);
  assert(unlimited_inner || isize <= max_inner);
  assert(osize >= min_outer);
  assert(unlimited_outer || osize <= max_outer);
  return oshape;
}

template <typename P, typename A, std::size_t D>
adt::range_t<D> nested_calc_outer_range(const adt::range_t<D> &inds) {
  bool unlimited_outer = fun_traits<P>::max_size == std::size_t(-1);
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  std::ptrdiff_t max_outer = unlimited_outer ? -1 : fun_traits<P>::max_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  adt::index_t<D> oshape = nested_calc_outer_shape<P, A>(inds);
  std::ptrdiff_t osize = adt::prod(oshape);
  adt::index_t<D> omin = inds.imin();
  adt::index_t<D> omax = osize == 0 ? inds.imin() : inds.imax();
  adt::index_t<D> ostep =
      unlimited_inner ? omax - omin : inds.istep() * max_inner;
  adt::range_t<D> oinds(omin, omax, ostep);
  assert(oinds.size() >= min_outer);
  assert(unlimited_outer || oinds.size() <= max_outer);
  assert(oinds.size() == osize);
  return oinds;
}

template <typename P, typename A, std::size_t D>
adt::range_t<D> nested_calc_inner_range(const adt::range_t<D> &inds,
                                        const adt::range_t<D> &oinds,
                                        const adt::index_t<D> &i) {
  std::ptrdiff_t min_outer = fun_traits<P>::min_size;
  bool unlimited_inner = fun_traits<A>::max_size == std::size_t(-1);
  std::ptrdiff_t min_inner = fun_traits<A>::min_size;
  std::ptrdiff_t max_inner = unlimited_inner ? -1 : fun_traits<A>::max_size;

  adt::index_t<D> imin = i;
  adt::index_t<D> imax = adt::min(imin + oinds.istep(), inds.imax());
  adt::index_t<D> istep = inds.istep();
  adt::range_t<D> iinds(imin, imax, istep);
  assert(iinds.size() >= min_inner);
  assert(unlimited_inner || iinds.size() <= max_inner);
  if (min_outer == 0)
    assert(!iinds.empty());
  return iinds;
}

template <typename CR> struct nested_iotaMapMulti : std::tuple<> {
  template <std::size_t D, typename F, typename... Args>
  auto operator()(const adt::index_t<D> &i, const adt::range_t<D> &inds,
                  const adt::range_t<D> &oinds, F &&f, Args &&... args) const {
    typedef typename CR::pointer_dummy P;
    typedef typename CR::array_dummy A;
    auto iinds = nested_calc_inner_range<P, A>(inds, oinds, i);
    return iotaMapMulti<A>(std::forward<F>(f), iinds,
                           std::forward<Args>(args)...);
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> *, typename R,
          typename CR>
CR iotaMapMulti(const typename CR::policy_type &policy, F &&f,
                const adt::range_t<D> &inds, Args &&... args) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  auto oinds = detail::nested_calc_outer_range<P, A>(inds);
  return CR{iotaMapMulti<P>(detail::nested_iotaMapMulti<CR>(), oinds, inds,
                            oinds, std::forward<F>(f),
                            std::forward<Args>(args)...),
            policy};
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

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args, typename C, typename R, typename CR>
CR fmap(F &&f, const adt::nested<P, A, T, Policy> &xss, Args &&... args) {
  return CR{fmap(detail::nested_fmap(), xss.data, std::forward<F>(f),
                 std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
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

template <typename F, typename P, typename A, typename T, typename Policy,
          typename T2, typename Policy2, typename... Args, typename C,
          typename R, typename CR>
CR fmap2(F &&f, const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, T2, Policy2> &yss, Args &&... args) {
  return CR{fmap2(detail::nested_fmap2(), xss.data, yss.data,
                  std::forward<F>(f), std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
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

template <typename F, typename P, typename A, typename T, typename Policy,
          typename T2, typename Policy2, typename T3, typename Policy3,
          typename... Args, typename C, typename R, typename CR>
CR fmap3(F &&f, const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, T2, Policy2> &yss,
         const adt::nested<P, A, T3, Policy3> &zss, Args &&... args) {
  return CR{fmap3(detail::nested_fmap3(), xss.data, yss.data, zss.data,
                  std::forward<F>(f), std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
}

// fmapStencil

namespace detail {
struct nested_fmapStencil_f : std::tuple<> {
  template <typename AT, typename BM, typename BP, typename F, typename G,
            typename... Args>
  auto operator()(AT &&xs, std::size_t bmask, BM &&bm, BP &&bp, F &&f, G &&g,
                  Args &&... args) const {
    return fmapStencil(std::forward<F>(f), std::forward<G>(g),
                       std::forward<AT>(xs), bmask, std::forward<BM>(bm),
                       std::forward<BP>(bp), std::forward<Args>(args)...);
  }
};
template <typename G> struct nested_fmapStencil_g {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename AT> auto operator()(AT &&xs, std::ptrdiff_t i) const {
    // TODO: This is slow; call boundaryMap instead
    return i == 0 ? cxx::invoke(g, head(std::forward<AT>(xs)), i)
                  : cxx::invoke(g, last(std::forward<AT>(xs)), i);
  }
};
}

template <typename F, typename G, typename P, typename A, typename T,
          typename Policy, typename BM, typename BP, typename... Args,
          typename C, typename B, typename R, typename CR>
CR fmapStencil(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
               std::size_t bmask, BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return CR{fmapStencil(detail::nested_fmapStencil_f(),
                        detail::nested_fmapStencil_g<std::decay_t<G>>{g},
                        xss.data, bmask, std::forward<BM>(bm),
                        std::forward<BP>(bp), std::forward<F>(f),
                        std::forward<G>(g), std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
}

namespace detail {
template <std::size_t> struct nested_fmapStencilMulti;
template <std::size_t> struct nested_fmapStencilMulti_f;
template <std::size_t, typename G> struct nested_fmapStencilMulti_g;
}

namespace detail {
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

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 0> *, typename CT, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, Args &&... args) {
  // cannot call fmap here
  assert(msize(xss.data) <= 1);
  return CR{fmap(detail::nested_fmapStencilMulti<D>(), xss.data,
                 std::forward<F>(f), std::forward<G>(g), bmask,
                 std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
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

template <> struct nested_fmapStencilMulti_f<1> : std::tuple<> {
  template <typename AT, typename BM0, typename BP0, typename F, typename G,
            typename... Args>
  auto operator()(AT &&xs, std::size_t bmask, BM0 &&bm0, BP0 &&bp0, F &&f,
                  G &&g, Args &&... args) const {
    return fmapStencilMulti<1>(std::forward<F>(f), std::forward<G>(g),
                               std::forward<AT>(xs), bmask,
                               std::forward<BM0>(bm0), std::forward<BP0>(bp0),
                               std::forward<Args>(args)...);
  }
};
template <typename G> struct nested_fmapStencilMulti_g<1, G> {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename AT> auto operator()(AT &&xs, std::ptrdiff_t i) const {
    return boundaryMap(g, std::forward<AT>(xs), i);
  }
};
}

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 1> *, typename CT, typename BC, typename B,
          typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bp0, Args &&... args) {
  return CR{fmapStencilMulti<D>(
                detail::nested_fmapStencilMulti_f<D>(),
                detail::nested_fmapStencilMulti_g<D, std::decay_t<G>>{g},
                xss.data, bmask, bm0.data, bp0.data, std::forward<F>(f),
                std::forward<G>(g), std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
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

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 2> *, typename CT, typename BC, typename B,
          typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bm1, const std::decay_t<BCB> &bp0,
                    const std::decay_t<BCB> &bp1, Args &&... args) {
  // cannot call fmap5 here
  assert(msize(xss.data) <= 1);
  return CR{fmap5(detail::nested_fmapStencilMulti<D>(), xss.data, bm0.data,
                  bm1.data, bp0.data, bp1.data, std::forward<F>(f),
                  std::forward<G>(g), bmask, std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
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

// Note: Why not use boundaryMap instead of (head|last)(fmap)?

template <typename P, typename A, typename T, typename Policy>
decltype(auto) head(const adt::nested<P, A, T, Policy> &xss) {
  return head(fmap(detail::nested_head(), xss.data));
}

template <typename P, typename A, typename T, typename Policy>
decltype(auto) last(const adt::nested<P, A, T, Policy> &xss) {
  return last(fmap(detail::nested_last(), xss.data));
}

// boundary

namespace detail {
struct nested_boundary {
  template <typename AT> auto operator()(AT &&xs, std::ptrdiff_t i) const {
    return boundary(std::forward<AT>(xs), i);
  }
};
}

template <typename P, typename A, typename T, typename Policy, typename CT,
          typename BC, typename BCT>
BCT boundary(const adt::nested<P, A, T, Policy> &xs, std::ptrdiff_t i) {
  // TODO: use boundaryMap instead
  return BCT{fmap(detail::nested_boundary(), boundary(xs.data, i), i),
             typename BCT::policy_type(xs.get_policy())};
}

// boudaryMap

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args, typename CT, typename BC, typename R, typename BCR>
BCR boundaryMap(F &&f, const adt::nested<P, A, T, Policy> &xs, std::ptrdiff_t i,
                Args &&... args) {
  // TODO: use boundaryMap instead
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// indexing

namespace detail {
struct nested_getIndex : std::tuple<> {
  template <typename AT>
  decltype(auto) operator()(AT &&xs, std::ptrdiff_t i) const {
    // return std::forward<AT>(xs)[i];
    return getIndex(std::forward<AT>(xs), i);
  }
};
}

template <typename P, typename A, typename T, typename Policy>
decltype(auto) getIndex(const adt::nested<P, A, T, Policy> &xs,
                        std::ptrdiff_t i) {
  // TODO: Can't just use mextract, container may be larger
  assert(msize(xs.data) <= 1);
  return mextract(fmap(detail::nested_getIndex(), xs.data, i));
}

template <typename P, typename A, typename T, typename Policy>
auto accumulator<adt::nested<P, A, T, Policy>>::finalize() -> CT {
  // TODO: Allow passing a policy
  return {munit<P>(data.finalize()), typename CT::policy_type()};
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
          typename T, typename Policy, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
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
          typename T, typename Policy, typename T2, typename Policy2,
          typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
           const adt::nested<P, A, T2, Policy2> &yss, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap2(detail::nested_foldMap2(), op, z, xss.data, yss.data,
                  std::forward<F>(f), op, z, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CT>
CT munit(const typename CT::policy_type &policy, T &&x) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CT{munit<P>(munit<A>(std::forward<T>(x))), policy};
}

// mjoin

namespace detail {
struct nested_mextract : std::tuple<> {
  // TODO: Why can't this be decltype(auto)? I assume a std::decay_t is
  // missing somewhere.
  template <typename NPAT> auto operator()(const NPAT &xss) const {
    // TODO: Can't just use mextract, container may be larger
    assert(msize(xss) <= 1);
    return mextract(xss.data);
  }
};
struct nested_mjoin : std::tuple<> {
  template <typename ANPAT> auto operator()(ANPAT &&xsss) const {
    return mjoin(fmap(nested_mextract(), std::forward<ANPAT>(xsss)));
  }
};
}

template <typename P, typename A, typename T, typename Policy, typename Policy2,
          typename CT>
CT mjoin(
    const adt::nested<P, A, adt::nested<P, A, T, Policy>, Policy2> &xssss) {
  // TODO: Don't invent a policy unless necessary
  return CT{fmap(detail::nested_mjoin(), xssss.data),
            typename CT::policy_type()};
}

// mbind

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args, typename CR>
CR mbind(F &&f, const adt::nested<P, A, T, Policy> &xss, Args &&... args) {
  static_assert(detail::is_nested<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xss, std::forward<Args>(args)...));
}

// mextract

template <typename P, typename A, typename T, typename Policy>
decltype(auto) mextract(const adt::nested<P, A, T, Policy> &xss) {
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
          typename T, typename Policy, typename... Args, typename C, typename R,
          typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
            Args &&... args) {
  return CR{fmap(detail::nested_mfoldMap(), xss.data, std::forward<F>(f),
                 std::forward<Op>(op), std::forward<Z>(z),
                 std::forward<Args>(args)...),
            typename CR::policy_type(xss.get_policy())};
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CR>
CR mzero(const typename CR::policy_type &policy) {
  typedef typename C::pointer_dummy P;
  typedef typename C::template array_constructor<R> AR;
  return CR{mzero<P, AR>(), policy};
}

// mplus

template <typename P, typename A, typename T, typename Policy, typename... Ts,
          typename... Policies, typename CT>
CT mplus(const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, Ts, Policies> &... yss) {
  typedef typename CT::template array_constructor<T> AT;
  // TODO: Can't just use mextract, container may be larger
  assert(msize(xss.data) <= 1);
  std::make_tuple((assert(msize(yss.data) <= 1), 0)...);
  return CT{munit<P, AT>(mplus(
                mempty(xss.data) ? mzero<A, T>() : mextract(xss.data),
                mempty(yss.data) ? mzero<A, T>() : mextract(yss.data)...)),
            xss.get_policy()};
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_nested<C>::value> *, typename CT>
CT msome(T &&x, Ts &&... ys) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  // TODO: don't invent a policy, allow passing one
  return CT{munit<P>(msome<A>(std::forward<T>(x), std::forward<Ts>(ys)...)),
            typename CT::policy_type()};
}

// mempty

template <typename P, typename A, typename T, typename Policy>
bool mempty(const adt::nested<P, A, T, Policy> &xss) {
  return mempty(xss.data);
}

// msize

template <typename P, typename A, typename T, typename Policy>
std::size_t msize(const adt::nested<P, A, T, Policy> &xss) {
  typedef
      typename adt::nested<P, A, T, Policy>::template array_constructor<T> AT;
  return mempty(xss.data) ? 0 : foldMap((std::size_t (*)(const AT &))msize,
                                        std::plus<std::size_t>(), 0, xss.data);
}
}

#define FUN_NESTED_IMPL_HPP_DONE
#endif // #ifdef FUN_NESTED_IMPL_HPP
#ifndef FUN_NESTED_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
