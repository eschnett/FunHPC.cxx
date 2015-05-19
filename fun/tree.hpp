#ifndef FUN_TREE_HPP
#define FUN_TREE_HPP

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
template <template <typename> class C, typename T>
struct is_tree<adt::tree<C, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <template <typename> class C, typename T>
struct fun_traits<adt::tree<C, T>> {
  template <typename U> using constructor = adt::tree<C, U>;
  typedef T value_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_tree<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return C<R>(typename C<R>::iotaMap(), std::forward<F>(f), 0, s, 1,
              std::forward<Args>(args)...);
}

// fmap

template <typename F, template <typename> class C, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  return adt::tree<C, R>(typename adt::tree<C, R>::fmap(), std::forward<F>(f),
                         xs, std::forward<Args>(args)...);
}

template <typename F, template <typename> class C, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const adt::tree<C, T> &xs, const adt::tree<C, T2> &ys,
           Args &&... args) {
  return adt::tree<C, R>(typename adt::tree<C, R>::fmap2(), std::forward<F>(f),
                         xs, ys, std::forward<Args>(args)...);
}

// fmapStencil

template <typename F, typename G, template <typename> class C, typename T,
          typename BM, typename BP, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>>
auto fmapStencil(F &&f, G &&g, const adt::tree<C, T> &xs, BM &&bm, BP &&bp,
                 Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return adt::tree<C, R>(typename adt::tree<C, R>::fmapStencil(),
                         std::forward<F>(f), std::forward<G>(g), xs, 0b11,
                         std::forward<BM>(bm), std::forward<BP>(bp),
                         std::forward<Args>(args)...);
}

// head, last

template <template <typename> class C, typename T>
auto head(const adt::tree<C, T> &xs) {
  return xs.head();
}

template <template <typename> class C, typename T>
auto last(const adt::tree<C, T> &xs) {
  return xs.last();
}

// foldMap

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
             Args &&... args) {
  return xs.foldMap(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
              const adt::tree<C, T2> &ys, Args &&... args) {
  return xs.foldMap2(std::forward<F>(f), std::forward<Op>(op),
                     std::forward<Z>(z), ys, std::forward<Args>(args)...);
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_tree<C<R>>::value> * = nullptr>
C<R> munit(T &&x) {
  return C<R>(std::forward<T>(x));
}

// mjoin

template <template <typename> class C, typename T>
adt::tree<C, T> mjoin(const adt::tree<C, adt::tree<C, T>> &xss) {
  return adt::tree<C, T>(typename adt::tree<C, T>::join(), xss);
}

// mbind

template <typename F, template <typename> class C, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>,
          typename R = typename CR::value_type>
adt::tree<C, R> mbind(F &&f, const adt::tree<C, T> &xs, Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <template <typename> class C, typename T>
auto mextract(const adt::tree<C, T> &xs) {
  return xs.head();
}

// mfoldMap

template <typename F, typename Op, typename Z, template <typename> class C,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
adt::tree<C, R> mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<C, T> &xs,
                         Args &&... args) {
  return munit<fun_traits<adt::tree<C, R>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_tree<C<R>>::value> * = nullptr>
C<R> mzero() {
  return C<R>();
}

// mplus

template <template <typename> class C, typename T, typename... Ts>
adt::tree<C, T> mplus(const adt::tree<C, T> &xss,
                      const adt::tree<C, Ts> &... yss) {
  return adt::tree<C, T>(xss, yss...);
}

// msome

template <template <typename> class C, typename T, typename... Ts,
          typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_tree<C<R>>::value> * = nullptr>
C<R> msome(T &&x, Ts &&... ys) {
  return C<R>{std::forward<T>(x), std::forward<Ts>(ys)...};
}

// mempty

template <template <typename> class C, typename T>
bool mempty(const adt::tree<C, T> &xs) {
  return xs.empty();
}
}

#define FUN_TREE_HPP_DONE
#endif // #ifdef FUN_TREE_HPP
#ifndef FUN_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
