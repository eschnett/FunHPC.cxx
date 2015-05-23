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
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR(typename CR::iotaMap(), std::forward<F>(f), 0, s, 1,
            std::forward<Args>(args)...);
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr>
auto iotaMap(F &&f, const typename fun_traits<C>::index_type &s,
             Args &&... args) {
  typedef typename fun_traits<C>::index_type Index;
  typedef cxx::invoke_of_t<F, Index, Args...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR(typename CR::iotaMap(), std::forward<F>(f), adt::zero<Index>(), s,
            adt::one<Index>(), std::forward<Args>(args)...);
}

// fmap

template <typename F, typename C, typename T, typename... Args>
auto fmap(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  return adt::tree<C, R>(typename adt::tree<C, R>::fmap(), std::forward<F>(f),
                         xs, std::forward<Args>(args)...);
}

template <typename F, typename C, typename T, typename T2, typename... Args>
auto fmap2(F &&f, const adt::tree<C, T> &xs, const adt::tree<C, T2> &ys,
           Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  return adt::tree<C, R>(typename adt::tree<C, R>::fmap2(), std::forward<F>(f),
                         xs, ys, std::forward<Args>(args)...);
}

// boundary

template <typename C, typename T>
auto boundary(const adt::tree<C, T> &xs, std::ptrdiff_t i) {
  typedef typename fun_traits<adt::tree<C, T>>::boundary_dummy BC;
  typedef typename fun_traits<BC>::template constructor<T> BCT;
  return BCT(typename BCT::boundary(), xs, i);
}

// boundaryMap

template <typename F, typename C, typename T, typename... Args>
auto boundaryMap(F &&f, const adt::tree<C, T> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  typedef typename fun_traits<adt::tree<C, T>>::boundary_dummy BC;
  typedef cxx::invoke_of_t<F, T, Args...> R;
  typedef typename fun_traits<BC>::template constructor<R> BCR;
  return BCR(typename BCR::boundaryMap(), std::forward<F>(f), xs, i,
             std::forward<Args>(args)...);
}

// fmapStencil

template <typename F, typename G, typename C, typename T, typename BM,
          typename BP, typename... Args>
auto fmapStencil(F &&f, G &&g, const adt::tree<C, T> &xs, BM &&bm, BP &&bp,
                 Args &&... args) {
  typedef cxx::invoke_of_t<G, T, std::ptrdiff_t> B;
  typedef cxx::invoke_of_t<F, T, std::size_t, B, B, Args...> R;
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return adt::tree<C, R>(typename adt::tree<C, R>::fmapStencil(),
                         std::forward<F>(f), std::forward<G>(g), xs, 0b11,
                         std::forward<BM>(bm), std::forward<BP>(bp),
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

// // indexing
//
// template <typename C, typename T>
// decltype(auto) getIndex(const adt::tree<C, T> &xs,std::ptrdiff_t i) {
//   return xs.getIndex(i);
// }

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
             Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename C, typename T,
          typename T2, typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
              const adt::tree<C, T2> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef std::decay_t<T> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR(std::forward<T>(x));
}

// mjoin

template <typename C, typename T>
auto mjoin(const adt::tree<C, adt::tree<C, T>> &xss) {
  return adt::tree<C, T>(typename adt::tree<C, T>::join(), xss);
}

// mbind

template <typename F, typename C, typename T, typename... Args>
auto mbind(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename C, typename T>
decltype(auto) mextract(const adt::tree<C, T> &xs) {
  return head(xs);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs,
              Args &&... args) {
  typedef typename fun_traits<adt::tree<A, T>>::dummy C;
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr>
auto mzero() {
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR();
}

// mplus

template <typename C, typename T, typename... Ts>
auto mplus(const adt::tree<C, T> &xss, const adt::tree<C, Ts> &... yss) {
  return adt::tree<C, T>(fun::msome<C>(xss, yss...));
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
  typedef std::decay_t<T> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
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
