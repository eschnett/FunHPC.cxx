#ifndef FUN_TREE_HPP
#define FUN_TREE_HPP

#include <adt/dummy.hpp>
#include <adt/tree.hpp>
#include <cxx/invoke.hpp>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fun {

// is_tree

namespace detail {
template <typename> struct is_tree : std::false_type {};
template <typename A, typename T>
struct is_tree<adt::tree<A, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename A, typename T> struct fun_traits<adt::tree<A, T>> {
  template <typename U> using constructor = adt::tree<A, U>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  typedef typename fun_traits<A>::index_type index_type;
  typedef adt::tree<typename fun_traits<A>::boundary_dummy, adt::dummy>
      boundary_dummy;
};

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return CR{CR::either_t::make_left(std::forward<T>(x))};
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  typedef typename CR::array_dummy A;
  return CR{CR::either_t::make_right(mzero<A, CR>())};
}

// iotaMap

namespace detail {
constexpr std::ptrdiff_t max_tree_size = 10;
template <std::size_t D>
const std::ptrdiff_t
    max_tree_linear_size = std::rint(std::pow(double(max_tree_size), 1.0 / D));
}

namespace detail {
template <typename C> struct tree_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::irange_t &inds,
                  std::ptrdiff_t scale, F &&f, Args &&... args) const;
};
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
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
  assert(scale < inds.shape() && scale * detail::max_tree_size >= inds.shape());
  adt::irange_t branch_inds(inds.imin(), inds.imax(), inds.istep() * scale);
  return CR{CR::either_t::make_right(
      iotaMap<A>(detail::tree_iotaMap<C>(), branch_inds, inds, scale,
                 std::forward<F>(f), std::forward<Args>(args)...))};
}

namespace detail {
template <typename C>
template <typename F, typename... Args>
auto tree_iotaMap<C>::operator()(std::ptrdiff_t i, const adt::irange_t &inds,
                                 std::ptrdiff_t scale, F &&f,
                                 Args &&... args) const {
  adt::irange_t sub_inds(i, std::min(i + inds.istep() * scale, inds.imax()),
                         inds.istep());
  return iotaMap<C>(std::forward<F>(f), sub_inds, std::forward<Args>(args)...);
}
}

namespace detail {
template <typename C> struct tree_iotaMapMulti : std::tuple<> {
  template <std::size_t D, typename F, typename... Args>
  auto operator()(const adt::index_t<D> &i, const adt::range_t<D> &inds,
                  std::ptrdiff_t scale, F &&f, Args &&... args) const;
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
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
  assert(
      adt::any(adt::lt(scale, inds.shape()) &&
               adt::ge(scale * detail::max_tree_linear_size<D>, inds.shape())));
  adt::range_t<D> branch_inds(inds.imin(), inds.imax(), inds.istep() * scale);
  return CR{CR::either_t::make_right(
      iotaMapMulti<A>(detail::tree_iotaMapMulti<C>(), branch_inds, inds, scale,
                      std::forward<F>(f), std::forward<Args>(args)...))};
}

namespace detail {
template <typename C>
template <std::size_t D, typename F, typename... Args>
auto tree_iotaMapMulti<C>::
operator()(const adt::index_t<D> &i, const adt::range_t<D> &inds,
           std::ptrdiff_t scale, F &&f, Args &&... args) const {
  adt::range_t<D> sub_inds(i, adt::min(i + inds.istep() * scale, inds.imax()),
                           inds.istep());
  return iotaMapMulti<C>(std::forward<F>(f), sub_inds,
                         std::forward<Args>(args)...);
}
}

// fmap

namespace detail {
struct tree_fmap : std::tuple<> {
  template <typename A, typename T, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, F &&f, Args &&... args) const;
};
}

template <typename F, typename A, typename T, typename... Args,
          typename CT = adt::tree<A, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
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
template <typename A, typename T, typename F, typename... Args>
auto tree_fmap::operator()(const adt::tree<A, T> &xs, F &&f,
                           Args &&... args) const {
  return fmap(std::forward<F>(f), xs, std::forward<Args>(args)...);
}
}

namespace detail {
struct tree_fmap2 : std::tuple<> {
  template <typename A, typename T, typename T2, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys, F &&f,
                  Args &&... args) const;
};
}

template <typename F, typename A, typename T, typename T2, typename... Args,
          typename CT = adt::tree<A, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
         Args &&... args) {
  bool s = xs.subtrees.right();
  assert(ys.subtrees.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                    ys.subtrees.get_left(), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmap2(
      detail::tree_fmap2(), xs.subtrees.get_right(), ys.subtrees.get_right(),
      std::forward<F>(f), std::forward<Args>(args)...))};
}

namespace detail {
template <typename A, typename T, typename T2, typename F, typename... Args>
auto tree_fmap2::operator()(const adt::tree<A, T> &xs,
                            const adt::tree<A, T2> &ys, F &&f,
                            Args &&... args) const {
  return fmap2(std::forward<F>(f), xs, ys, std::forward<Args>(args)...);
}
}

// head, last

template <typename A, typename T> T head(const adt::tree<A, T> &xs) {
// TODO: head(head(...)) copies subtrees; call boundaryMap instead
#warning "TODO: This is slow; call boundaryMap instead"
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
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const;
};
}

template <typename A, typename T, typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::tree<A, T> &xs, std::ptrdiff_t i) {
  bool s = xs.subtrees.right();
  if (!s)
    return munit<BC>(xs.subtrees.get_left());
  // TODO: use boundaryMap instead to avoid copying subtrees
  return BCT{BCT::either_t::make_right(
      fmap(detail::tree_boundary(), boundary(xs.subtrees.get_right(), i), i))};
}

namespace detail {
template <typename A, typename T>
auto tree_boundary::operator()(const adt::tree<A, T> &xs,
                               std::ptrdiff_t i) const {
  return boundary(xs, i);
}
}

// boundaryMap

namespace detail {
struct tree_boundaryMap : std::tuple<> {
  template <typename A, typename T, typename F, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i, F &&f,
                  Args &&... args) const;
};
}

template <typename F, typename A, typename T, typename... Args,
          typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
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

namespace detail {
template <typename A, typename T, typename F, typename... Args>
auto tree_boundaryMap::operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i,
                                  F &&f, Args &&... args) const {
  return boundaryMap(std::forward<F>(f), xs, i, std::forward<Args>(args)...);
}
}

// fmapStencil

namespace detail {
struct tree_fmapStencil_f : std::tuple<> {
  template <typename A, typename T, typename BM, typename BP, typename F,
            typename G, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, std::size_t bmask, BM &&bm,
                  BP &&bp, F &&f, G &&g, Args &&... args) const;
};
template <typename G> struct tree_fmapStencil_g {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const;
};
}

template <typename F, typename G, typename A, typename T, typename BM,
          typename BP, typename... Args, typename CT = adt::tree<A, T>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
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
template <typename A, typename T, typename BM, typename BP, typename F,
          typename G, typename... Args>
auto tree_fmapStencil_f::operator()(const adt::tree<A, T> &xs,
                                    std::size_t bmask, BM &&bm, BP &&bp, F &&f,
                                    G &&g, Args &&... args) const {
  return fmapStencil(std::forward<F>(f), std::forward<G>(g), xs, bmask,
                     std::forward<BM>(bm), std::forward<BP>(bp),
                     std::forward<Args>(args)...);
}
template <typename G>
template <typename A, typename T>
auto tree_fmapStencil_g<G>::operator()(const adt::tree<A, T> &xs,
                                       std::ptrdiff_t i) const {
#warning "TODO: This is slow; call boundaryMap instead"
  return i == 0 ? cxx::invoke(g, head(xs), i) : cxx::invoke(g, last(xs), i);
}
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
                  Args &&... args) const;
};
template <typename G> struct tree_fmapStencilMulti_g<1, G> {
  G g;
  template <typename Archive> void serialize(Archive &ar) { ar(g); }
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs, std::ptrdiff_t i) const;
};
}

template <std::size_t D, typename F, typename G, typename A, typename T,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<A, T> &xs, std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    Args &&... args) {
  bool s = xs.subtrees.right();
  assert(bm0.subtrees.right() == s);
  assert(bp0.subtrees.right() == s);
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
template <typename A, typename T, typename AM0, typename BM0, typename AP0,
          typename BP0, typename F, typename G, typename... Args>
auto tree_fmapStencilMulti_f<1>::
operator()(const adt::tree<A, T> &xs, std::size_t bmask,
           const adt::tree<AM0, BM0> &bm0, const adt::tree<AP0, BP0> &bp0,
           F &&f, G &&g, Args &&... args) const {
  return fmapStencilMulti<1>(std::forward<F>(f), std::forward<G>(g), xs, bmask,
                             bm0, bp0, std::forward<Args>(args)...);
}
template <typename G>
template <typename A, typename T>
auto tree_fmapStencilMulti_g<1, G>::operator()(const adt::tree<A, T> &xs,
                                               std::ptrdiff_t i) const {
  return boundaryMap(g, xs, i);
}
}

// foldMap

namespace detail {
struct tree_foldMap {
  template <typename A, typename T, typename F, typename Op, typename Z,
            typename... Args>
  auto operator()(const adt::tree<A, T> &xs, F &&f, Op &&op, Z &&z,
                  Args &&... args) const;
};
}

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
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
template <typename A, typename T, typename F, typename Op, typename Z,
          typename... Args>
auto tree_foldMap::operator()(const adt::tree<A, T> &xs, F &&f, Op &&op, Z &&z,
                              Args &&... args) const {
  return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                 xs, std::forward<Args>(args)...);
}
}

namespace detail {
struct tree_foldMap2 : std::tuple<> {
  template <typename A, typename T, typename T2, typename F, typename Op,
            typename Z, typename... Args>
  auto operator()(const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys, F &&f,
                  Op &&op, Z &&z, Args &&... args) const;
};
}

template <typename F, typename Op, typename Z, typename A, typename T,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs,
           const adt::tree<A, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.subtrees.right();
  assert(ys.subtrees.right() == s);
  if (!s)
    return cxx::invoke(std::forward<F>(f), xs.subtrees.get_left(),
                       ys.subtrees.get_left(), std::forward<Args>(args)...);
  return foldMap2(detail::tree_foldMap2(), op, z, xs.subtrees.get_right(),
                  ys.subtrees.get_right(), f, op, z,
                  std::forward<Args>(args)...);
}

namespace detail {
template <typename A, typename T, typename T2, typename F, typename Op,
          typename Z, typename... Args>
auto tree_foldMap2::operator()(const adt::tree<A, T> &xs,
                               const adt::tree<A, T2> &ys, F &&f, Op &&op,
                               Z &&z, Args &&... args) const {
  return foldMap2(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                  xs, ys, std::forward<Args>(args)...);
}
}

// mjoin

namespace detail {
struct tree_mjoin {
  template <typename A, typename T>
  auto operator()(const adt::tree<A, T> &xs) const;
};
}

template <typename A, typename T, typename CT = adt::tree<A, T>>
CT mjoin(const adt::tree<A, adt::tree<A, T>> &xss) {
  bool s = xss.subtrees.right();
  if (!s)
    return xss.subtrees.get_left();
  return CT{CT::either_t::make_right(
      fmap(detail::tree_mjoin(), xss.subtrees.get_right()))};
}

namespace detail {
template <typename A, typename T>
auto tree_mjoin::operator()(const adt::tree<A, T> &xs) const {
  return mjoin(xs);
}
}

// mbind

template <typename F, typename A, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
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
          typename... Args, typename C = adt::tree<A, adt::dummy>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs, Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mplus

template <typename A, typename T, typename... Ts, typename CT = adt::tree<A, T>>
CT mplus(const adt::tree<A, T> &xss, const adt::tree<A, Ts> &... yss) {
  return CT{CT::either_t::make_right(fun::msome<A>(xss, yss...))};
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR msome(T &&x, Ts &&... ys) {
  typedef typename CR::array_dummy A;
  return CR{CR::either_t::make_right(msome<A>(
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
  auto operator()(const adt::tree<A, T> &xs) const;
};
}

template <typename A, typename T> std::size_t msize(const adt::tree<A, T> &xs) {
  bool s = xs.subtrees.right();
  if (!s)
    return 1;
  return foldMap(detail::tree_msize(), std::plus<std::size_t>(), 0,
                 xs.subtrees.get_right());
}

namespace detail {
template <typename A, typename T>
auto tree_msize::operator()(const adt::tree<A, T> &xs) const {
  return msize(xs);
}
}
}

#define FUN_TREE_HPP_DONE
#endif // #ifdef FUN_TREE_HPP
#ifndef FUN_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
