#ifndef FUN_TREE_HPP
#define FUN_TREE_IMPL_HPP

#include "tree_decl.hpp"

#include <adt/dummy.hpp>
#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <adt/tree_impl.hpp>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fun {

// iotaMap

namespace detail {
constexpr std::ptrdiff_t max_tree_size = 16;
template <std::size_t D>
const std::ptrdiff_t
    max_tree_linear_size = std::rint(std::pow(double(max_tree_size), 1.0 / D));
}

namespace detail {
template <typename C> struct tree_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::irange_t &inds,
                  std::ptrdiff_t scale, F &&f, Args &&... args) const {
    adt::irange_t sub_inds(i, std::min(i + inds.istep() * scale, inds.imax()),
                           inds.istep());
    return iotaMap<C>(std::forward<F>(f), sub_inds,
                      std::forward<Args>(args)...);
  }
};
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> *, typename R,
          typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  typedef typename CR::array_dummy A;
  // Empty tree: special case
  if (inds.empty())
    return mzero<C, R>();
  // Leaf
  if (inds.shape() == 1)
    return munit<C>(
        cxx::invoke(std::forward<F>(f), inds[0], std::forward<Args>(args)...));
  // Branch
  // Calculate optimal branch size, so that all sub-branches will be full
  std::ptrdiff_t scale = 1;
  while (inds.shape() > scale * detail::max_tree_size)
    scale *= detail::max_tree_size;
  cxx_assert(scale < inds.shape() &&
             scale * detail::max_tree_size >= inds.shape());
  adt::irange_t branch_inds(inds.imin(), inds.imax(), inds.istep() * scale);
  return CR{CR::either_t::make_right(
      iotaMap<A>(detail::tree_iotaMap<C>(), branch_inds, inds, scale,
                 std::forward<F>(f), std::forward<Args>(args)...))};
}

#if 0
namespace detail {
template <typename C> struct tree_iotaMapMulti : std::tuple<> {
  template <std::size_t D, typename F, typename... Args>
  auto operator()(const adt::index_t<D> &i, const adt::range_t<D> &inds,
                  std::ptrdiff_t scale, F &&f, Args &&... args) const {
    adt::range_t<D> sub_inds(i, adt::min(i + inds.istep() * scale, inds.imax()),
                             inds.istep());
    return iotaMapMulti<C>(std::forward<F>(f), sub_inds,
                           std::forward<Args>(args)...);
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> *, typename R,
          typename CR>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  typedef typename CR::array_dummy A;
  // Empty tree: special case
  if (inds.empty())
    return mzero<C, R>();
  // Leaf
  if (inds.size() == 1)
    return munit<C>(cxx::invoke(std::forward<F>(f), inds.imin(),
                                std::forward<Args>(args)...));
  // Branch
  // Calculate optimal branch size, so that all sub-branches will be full
  // TODO: Consider different scales in different directions
  std::ptrdiff_t scale = 1;
  while (
      adt::any(adt::gt(inds.shape(), scale * detail::max_tree_linear_size<D>)))
    scale *= detail::max_tree_linear_size<D>;
  cxx_assert(
      adt::any(adt::lt(scale, inds.shape()) &&
               adt::ge(scale * detail::max_tree_linear_size<D>, inds.shape())));
  adt::range_t<D> branch_inds(inds.imin(), inds.imax(), inds.istep() * scale);
  return CR{CR::either_t::make_right(
      iotaMapMulti<A>(detail::tree_iotaMapMulti<C>(), branch_inds, inds, scale,
                      std::forward<F>(f), std::forward<Args>(args)...))};
}
#endif

namespace detail {
template <typename C> struct tree_iotaMapMulti_leaf : std::tuple<> {
  template <std::size_t D, typename F, typename... Args,
            typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
            typename CR = typename fun_traits<C>::template constructor<R>>
  auto operator()(const adt::index_t<D> &i, F &&f, Args &&... args) const {
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), i, std::forward<Args>(args)...))};
  }
};

template <typename C> struct tree_iotaMapMulti_branch : std::tuple<> {
  template <std::size_t D, typename F, typename... Args,
            typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
            typename CR = typename fun_traits<C>::template constructor<R>>
  auto operator()(const adt::index_t<D> &i, const adt::range_t<D> &all_inds,
                  std::ptrdiff_t scale, F &&f, Args &&... args) const {
    typedef typename CR::array_dummy A;
    adt::range_t<D> inds(
        i, adt::min(i + all_inds.istep() * scale, all_inds.imax()),
        all_inds.istep());
    cxx_assert(scale % detail::max_tree_linear_size<D> == 0);
    scale /= detail::max_tree_linear_size<D>;
    if (scale == 1)
      return CR{CR::either_t::make_right(
          iotaMapMulti<A>(tree_iotaMapMulti_leaf<C>(), inds, std::forward<F>(f),
                          std::forward<Args>(args)...))};
    adt::range_t<D> branch_inds(inds.imin(), inds.imax(), inds.istep() * scale);
    return CR{CR::either_t::make_right(
        iotaMapMulti<A>(tree_iotaMapMulti_branch<C>(), branch_inds, inds, scale,
                        std::forward<F>(f), std::forward<Args>(args)...))};
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> *, typename R,
          typename CR>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  // Empty tree: special case
  if (inds.empty())
    return mzero<C, R>();
  // Leaf
  if (inds.size() == 1)
    return detail::tree_iotaMapMulti_leaf<C>()(inds.imin(), std::forward<F>(f),
                                               std::forward<Args>(args)...);
  // Calculate optimal branch size, so that all sub-branches will be full
  // TODO: Consider different scales in different directions
  std::ptrdiff_t scale = 1;
  while (
      adt::any(adt::gt(inds.shape(), scale * detail::max_tree_linear_size<D>)))
    scale *= detail::max_tree_linear_size<D>;
  cxx_assert(
      adt::any(adt::lt(scale, inds.shape()) &&
               adt::ge(scale * detail::max_tree_linear_size<D>, inds.shape())));
  return detail::tree_iotaMapMulti_branch<C>()(
      inds.imin(), inds, scale * detail::max_tree_linear_size<D>,
      std::forward<F>(f), std::forward<Args>(args)...);
}

// fmap

namespace detail {
struct tree_fmap : std::tuple<> {
  template <typename A, typename T, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, F &&f, Args &&... args) const {
    return fmap(std::forward<F>(f), xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename A, typename T, typename... Args, typename CT,
          typename R, typename CR>
CR fmap(F &&f, const adt::tree<A, T> &xs, Args &&... args) {
  bool s = xs.subtrees.right();
  if (!s)
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                    std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(
      fmap(detail::tree_fmap(), xs.subtrees.get_right(), std::forward<F>(f),
           std::forward<Args>(args)...))};
}

namespace detail {
struct tree_fmap2 : std::tuple<> {
  template <typename A, typename T, typename T2, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys, F &&f,
                  Args &&... args) const {
    return fmap2(std::forward<F>(f), xs, ys, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename A, typename T, typename T2, typename... Args,
          typename CT, typename R, typename CR>
CR fmap2(F &&f, const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
         Args &&... args) {
  bool s = xs.subtrees.right();
  cxx_assert(ys.subtrees.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                    ys.subtrees.get_left(), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmap2(
      detail::tree_fmap2(), xs.subtrees.get_right(), ys.subtrees.get_right(),
      std::forward<F>(f), std::forward<Args>(args)...))};
}

namespace detail {
struct tree_fmap3 : std::tuple<> {
  template <typename A, typename T, typename T2, typename T3, typename F,
            typename... Args>
  auto operator()(const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
                  const adt::tree<A, T3> &zs, F &&f, Args &&... args) const {
    return fmap3(std::forward<F>(f), xs, ys, zs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename A, typename T, typename T2, typename T3,
          typename... Args, typename CT, typename R, typename CR>
CR fmap3(F &&f, const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
         const adt::tree<A, T3> &zs, Args &&... args) {
  bool s = xs.subtrees.right();
  cxx_assert(ys.subtrees.right() == s);
  cxx_assert(zs.subtrees.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(cxx::invoke(
        std::forward<F>(f), xs.subtrees.get_left(), ys.subtrees.get_left(),
        zs.subtrees.get_left(), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(
      fmap3(detail::tree_fmap3(), xs.subtrees.get_right(),
            ys.subtrees.get_right(), zs.subtrees.get_right(),
            std::forward<F>(f), std::forward<Args>(args)...))};
}

// head, last

template <typename A, typename T> T head(const adt::tree<A, T> &xs) {
  // TODO: head(head(...)) copies subtrees; call boundaryMap instead
  // TODO: This is slow; call boundaryMap instead
  return xs.subtrees.left() ? xs.subtrees.get_left()
                            : head(head(xs.subtrees.get_right()));
}

template <typename A, typename T> T last(const adt::tree<A, T> &xs) {
  return xs.subtrees.left() ? xs.subtrees.get_left()
                            : last(last(xs.subtrees.get_right()));
}

// boundary

namespace detail {
struct tree_boundary : std::tuple<> {
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const {
    return boundary(xs, i);
  }
};
}

template <typename A, typename T, typename CT, typename BC, typename BCT>
BCT boundary(const adt::tree<A, T> &xs, std::ptrdiff_t i) {
  bool s = xs.subtrees.right();
  if (!s)
    return munit<BC>(xs.subtrees.get_left());
  // TODO: use boundaryMap instead to avoid copying subtrees
  return BCT{BCT::either_t::make_right(
      fmap(detail::tree_boundary(), boundary(xs.subtrees.get_right(), i), i))};
}

// boundaryMap

namespace detail {
struct tree_boundaryMap : std::tuple<> {
  template <typename A, typename T, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i, F &&f,
                  Args &&... args) const {
    return boundaryMap(std::forward<F>(f), xs, i, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename A, typename T, typename... Args, typename CT,
          typename BC, typename R, typename BCR>
BCR boundaryMap(F &&f, const adt::tree<A, T> &xs, std::ptrdiff_t i,
                Args &&... args) {
  bool s = xs.subtrees.right();
  if (!s)
    return munit<BC>(cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(), i,
                                 std::forward<Args>(args)...));
  return BCR{BCR::either_t::make_right(
      boundaryMap(detail::tree_boundaryMap(), xs.subtrees.get_right(), i,
                  std::forward<F>(f), std::forward<Args>(args)...))};
}

// fmapStencil

namespace detail {
struct tree_fmapStencil_f : std::tuple<> {
  template <typename A, typename T, typename BM, typename BP, typename F,
            typename G, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, std::size_t bmask, BM &&bm,
                  BP &&bp, F &&f, G &&g, Args &&... args) const {
    return fmapStencil(std::forward<F>(f), std::forward<G>(g), xs, bmask,
                       std::forward<BM>(bm), std::forward<BP>(bp),
                       std::forward<Args>(args)...);
  }
};
template <typename G> struct tree_fmapStencil_g {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const {
    // TODO: This is slow; call boundaryMap instead
    return i == 0 ? cxx::invoke(g, head(xs), i) : cxx::invoke(g, last(xs), i);
  }
};
}

template <typename F, typename G, typename A, typename T, typename BM,
          typename BP, typename... Args, typename CT, typename B, typename R,
          typename CR>
CR fmapStencil(F &&f, const G &g, const adt::tree<A, T> &xs, std::size_t bmask,
               BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  bool s = xs.subtrees.right();
  if (!s)
    return CR{CR::either_t::make_left(cxx::invoke(
        std::forward<F>(f), xs.subtrees.get_left(), bmask, std::forward<BM>(bm),
        std::forward<BP>(bp), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmapStencil(
      detail::tree_fmapStencil_f(),
      detail::tree_fmapStencil_g<std::decay_t<G>>{g}, xs.subtrees.get_right(),
      bmask, std::forward<BM>(bm), std::forward<BP>(bp), std::forward<F>(f), g,
      std::forward<Args>(args)...))};
}

namespace detail {
template <std::size_t D> struct tree_fmapStencilMulti_f;
template <std::size_t D, typename G> struct tree_fmapStencilMulti_g;
}

namespace detail {
template <> struct tree_fmapStencilMulti_f<1> : std::tuple<> {
  template <typename A, typename T, typename AM0, typename BM0, typename AP0,
            typename BP0, typename F, typename G, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, std::size_t bmask,
                  const adt::tree<AM0, BM0> &bm0,
                  const adt::tree<AP0, BP0> &bp0, F &&f, G &&g,
                  Args &&... args) const {
    return fmapStencilMulti<1>(std::forward<F>(f), std::forward<G>(g), xs,
                               bmask, bm0, bp0, std::forward<Args>(args)...);
  }
};
template <typename G> struct tree_fmapStencilMulti_g<1, G> {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const {
    return boundaryMap(g, xs, i);
  }
};
}

template <std::size_t D, typename F, typename G, typename A, typename T,
          typename... Args, std::enable_if_t<D == 1> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<A, T> &xs, std::size_t bmask,
                    const std::decay_t<BCB> &bm0, const std::decay_t<BCB> &bp0,
                    Args &&... args) {
  bool s = xs.subtrees.right();
  cxx_assert(bm0.subtrees.right() == s);
  cxx_assert(bp0.subtrees.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(), bmask,
                    bm0.subtrees.get_left(), bp0.subtrees.get_left(),
                    std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmapStencilMulti<D>(
      detail::tree_fmapStencilMulti_f<D>(),
      detail::tree_fmapStencilMulti_g<D, std::decay_t<G>>{g},
      xs.subtrees.get_right(), bmask, bm0.subtrees.get_right(),
      bp0.subtrees.get_right(), std::forward<F>(f), g,
      std::forward<Args>(args)...))};
}

namespace detail {
template <> struct tree_fmapStencilMulti_f<2> : std::tuple<> {
  template <typename A, typename T, typename AM0, typename BM0, typename AM1,
            typename BM1, typename AP0, typename BP0, typename AP1,
            typename BP1, typename F, typename G, typename... Args>
  auto
  operator()(const adt::tree<A, T> &xs, std::size_t bmask,
             const adt::tree<AM0, BM0> &bm0, const adt::tree<AM1, BM1> &bm1,
             const adt::tree<AP0, BP0> &bp0, const adt::tree<AP1, BP1> &bp1,
             F &&f, G &&g, Args &&... args) const {
    return fmapStencilMulti<2>(std::forward<F>(f), std::forward<G>(g), xs,
                               bmask, bm0, bm1, bp0, bp1,
                               std::forward<Args>(args)...);
  }
};
template <typename G> struct tree_fmapStencilMulti_g<2, G> {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const {
    return boundaryMap(g, xs, i);
  }
};
}

template <std::size_t D, typename F, typename G, typename A, typename T,
          typename... Args, std::enable_if_t<D == 2> *, typename CT,
          typename BC, typename B, typename BCB, typename R, typename CR>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<A, T> &xs, std::size_t bmask,
                    const std::decay_t<BCB> &bm0, const std::decay_t<BCB> &bm1,
                    const std::decay_t<BCB> &bp0, const std::decay_t<BCB> &bp1,
                    Args &&... args) {
  bool s = xs.subtrees.right();
  cxx_assert(bm0.subtrees.right() == s);
  cxx_assert(bm1.subtrees.right() == s);
  cxx_assert(bp0.subtrees.right() == s);
  cxx_assert(bp1.subtrees.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(), bmask,
                    bm0.subtrees.get_left(), bm1.subtrees.get_left(),
                    bp0.subtrees.get_left(), bp1.subtrees.get_left(),
                    std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmapStencilMulti<D>(
      detail::tree_fmapStencilMulti_f<D>(),
      detail::tree_fmapStencilMulti_g<D, std::decay_t<G>>{g},
      xs.subtrees.get_right(), bmask, bm0.subtrees.get_right(),
      bm1.subtrees.get_right(), bp0.subtrees.get_right(),
      bp1.subtrees.get_right(), std::forward<F>(f), g,
      std::forward<Args>(args)...))};
}

// foldMap

namespace detail {
struct tree_foldMap {
  template <typename A, typename T, typename F, typename Op, typename Z,
            typename... Args>
  auto operator()(const adt::tree<A, T> &xs, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                   xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.subtrees.right();
  if (!s)
    return cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                       std::forward<Args>(args)...);
  return foldMap(detail::tree_foldMap(), op, z, xs.subtrees.get_right(), f, op,
                 z, std::forward<Args>(args)...);
}

namespace detail {
struct tree_foldMap2 : std::tuple<> {
  template <typename A, typename T, typename T2, typename F, typename Op,
            typename Z, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys, F &&f,
                  Op &&op, Z &&z, Args &&... args) const {
    return foldMap2(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), xs, ys, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename A, typename T,
          typename T2, typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs,
           const adt::tree<A, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.subtrees.right();
  cxx_assert(ys.subtrees.right() == s);
  if (!s)
    return cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                       ys.subtrees.get_left(), std::forward<Args>(args)...);
  return foldMap2(detail::tree_foldMap2(), op, z, xs.subtrees.get_right(),
                  ys.subtrees.get_right(), f, op, z,
                  std::forward<Args>(args)...);
}

// dump

namespace detail {
template <typename A, typename T> ostreamer dump(const adt::tree<A, T> &xs) {
  bool s = xs.subtrees.right();
  if (!s) {
    std::ostringstream os;
    os << "leaf{" << xs.subtrees.get_left() << "},";
    return ostreamer(os.str());
  }
  return ostreamer("branch{") +
         foldMap([](const auto &xs) { return dump(xs); }, combine_ostreamers(),
                 ostreamer(), xs.subtrees.get_right()) +
         ostreamer("},");
}
}

template <typename A, typename T> ostreamer dump(const adt::tree<A, T> &xs) {
  return ostreamer("tree{") + detail::dump(xs) + ostreamer("}");
}

// munit

template <typename C, typename T, std::enable_if_t<detail::is_tree<C>::value> *,
          typename CT>
CT munit(T &&x) {
  return CT{CT::either_t::make_left(std::forward<T>(x))};
}

// mzero

template <typename C, typename R, std::enable_if_t<detail::is_tree<C>::value> *,
          typename CR>
CR mzero() {
  typedef typename CR::array_dummy A;
  return CR{CR::either_t::make_right(mzero<A, CR>())};
}

// mjoin

namespace detail {
struct tree_mjoin {
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs) const {
    return mjoin(xs);
  }
};
}

template <typename A, typename T, typename CT>
CT mjoin(const adt::tree<A, adt::tree<A, T>> &xss) {
  bool s = xss.subtrees.right();
  if (!s)
    return xss.subtrees.get_left();
  return CT{CT::either_t::make_right(
      fmap(detail::tree_mjoin(), xss.subtrees.get_right()))};
}

// mbind

template <typename F, typename A, typename T, typename... Args, typename CR>
CR mbind(F &&f, const adt::tree<A, T> &xs, Args &&... args) {
  static_assert(detail::is_tree<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename A, typename T>
decltype(auto) mextract(const adt::tree<A, T> &xs) {
  return head(xs);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args, typename C, typename R, typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs, Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mplus

template <typename A, typename T, typename... Ts, typename CT>
CT mplus(const adt::tree<A, T> &xss, const adt::tree<A, Ts> &... yss) {
  return CT{CT::either_t::make_right(msome<A>(xss, yss...))};
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_tree<C>::value> *, typename CT>
CT msome(T &&x, Ts &&... ys) {
  typedef typename CT::array_dummy A;
  return CT{CT::either_t::make_right(msome<A>(
      munit<C>(std::forward<T>(x)), munit<C>(std::forward<Ts>(ys))...))};
}

// mempty

template <typename A, typename T> bool mempty(const adt::tree<A, T> &xs) {
  bool s = xs.subtrees.right();
  if (!s)
    return false;
  // Note: This treats that consist of multiple levels of emtpy
  // subtrees as non-empty
  return mempty(xs.subtrees.get_right());
}

// msize

namespace detail {
struct tree_msize : std::tuple<> {
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs) const {
    return msize(xs);
  }
};
}

template <typename A, typename T> std::size_t msize(const adt::tree<A, T> &xs) {
  bool s = xs.subtrees.right();
  if (!s)
    return 1;
  return foldMap(detail::tree_msize(), std::plus<std::size_t>(), 0,
                 xs.subtrees.get_right());
}
}

#define FUN_TREE_IMPL_HPP_DONE
#endif // #ifdef FUN_TREE_IMPL_HPP
#ifndef FUN_TREE_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
