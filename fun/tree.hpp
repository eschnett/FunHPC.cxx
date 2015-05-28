#ifndef FUN_TREE_HPP
#define FUN_TREE_HPP

#include <adt/dummy.hpp>
#include <adt/tree.hpp>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_tree

namespace detail {
template <typename> struct is_tree : std::false_type {};
template <typename C, typename T>
struct is_tree<adt::tree<C, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename C, typename T> struct fun_traits<adt::tree<C, T>> {
  template <typename U> using constructor = adt::tree<C, U>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<C>::rank;
  typedef typename fun_traits<C>::index_type index_type;
  typedef adt::tree<typename fun_traits<C>::boundary_dummy, adt::dummy>
      boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return CR(typename CR::iotaMap(), std::forward<F>(f), 0, s, 1,
            std::forward<Args>(args)...);
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::index_t<D> &s, Args &&... args) {
  return CR(typename CR::iotaMapMulti(), std::forward<F>(f),
            adt::zero<adt::index_t<D>>(), s, adt::one<adt::index_t<D>>(),
            std::forward<Args>(args)...);
}

// fmap

template <typename F, typename C, typename T, typename... Args,
          typename CT = adt::tree<C, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  return CR(typename CR::fmap(), std::forward<F>(f), xs,
            std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, typename T2, typename... Args,
          typename CT = adt::tree<C, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::tree<C, T> &xs, const adt::tree<C, T2> &ys,
         Args &&... args) {
  return CR(typename CR::fmap2(), std::forward<F>(f), xs, ys,
            std::forward<Args>(args)...);
}

// fmapStencil

template <typename F, typename G, typename C, typename T, typename BM,
          typename BP, typename... Args, typename CT = adt::tree<C, T>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const adt::tree<C, T> &xs, BM &&bm, BP &&bp,
               Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return CR(typename CR::fmapStencil(), std::forward<F>(f), std::forward<G>(g),
            xs, 0b11, std::forward<BM>(bm), std::forward<BP>(bp),
            std::forward<Args>(args)...);
}

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::tree<C, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<C, T> &xs, const BCB &bm0,
                    const BCB &bp0, Args &&... args) {
  return CR(typename CR::fmapStencilMulti(),
            std::integral_constant<std::size_t, D>(), std::forward<F>(f),
            std::forward<G>(g), xs, 0b11, bm0, bp0,
            std::forward<Args>(args)...);
}

// head, last

template <typename C, typename T>
decltype(auto) head(const adt::tree<C, T> &xs) {
  return xs.head();
}

template <typename C, typename T>
decltype(auto) last(const adt::tree<C, T> &xs) {
  return xs.last();
}

// boundary

template <typename C, typename T, typename CT = adt::tree<C, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::tree<C, T> &xs, std::ptrdiff_t i) {
  return BCT(typename BCT::boundary(), xs, i);
}

// boundaryMap

template <typename F, typename C, typename T, typename... Args,
          typename CT = adt::tree<C, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::tree<C, T> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return BCR(typename BCR::boundaryMap(), std::forward<F>(f), xs, i,
             std::forward<Args>(args)...);
}

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs, Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename C, typename T,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
           const adt::tree<C, T2> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return CR(std::forward<T>(x));
}

// mjoin

template <typename C, typename T, typename CT = adt::tree<C, T>>
CT mjoin(const adt::tree<C, adt::tree<C, T>> &xss) {
  return CT(typename CT::join(), xss);
}

// mbind

template <typename F, typename C, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  static_assert(detail::is_tree<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename C, typename T>
decltype(auto) mextract(const adt::tree<C, T> &xs) {
  return head(xs);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          typename... Args, typename CT = adt::tree<C, adt::dummy>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs, Args &&... args) {
  return munit<typename fun_traits<CT>::dummy>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return CR();
}

// mplus

template <typename C, typename T, typename... Ts, typename CT = adt::tree<C, T>>
CT mplus(const adt::tree<C, T> &xss, const adt::tree<C, Ts> &... yss) {
  return CT(fun::msome<C>(xss, yss...));
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR msome(T &&x, Ts &&... ys) {
  return CR(msome<typename CR::container_dummy>(std::forward<T>(x),
                                                std::forward<Ts>(ys)...));
}

// mempty

template <typename C, typename T> bool mempty(const adt::tree<C, T> &xs) {
  return xs.empty();
}

// msize

template <typename C, typename T> std::size_t msize(const adt::tree<C, T> &xs) {
  return xs.size();
}
}

#define FUN_TREE_HPP_DONE
#endif // #ifdef FUN_TREE_HPP
#ifndef FUN_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
