#ifndef FUN_SHARED_PTR_HPP
#define FUN_SHARED_PTR_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace fun {

// is_shared_ptr

namespace detail {
template <typename> struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<std::shared_ptr<T>> {
  template <typename U> using constructor = std::shared_ptr<std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;
};

// iotaMap

template <
    typename C, typename F, typename... Args,
    std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr,
    typename R = std::decay_t<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  assert(inds.size() <= 1);
  if (__builtin_expect(inds.empty(), false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), inds[0], std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args,
          typename C = std::shared_ptr<T>,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, Args...>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = std::shared_ptr<T>,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, T2, Args...>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const std::shared_ptr<T> &xs, const std::shared_ptr<T2> &ys,
         Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, *ys, std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename T3, typename... Args,
          typename C = std::shared_ptr<T>,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, T2, T3, Args...>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const std::shared_ptr<T> &xs, const std::shared_ptr<T2> &ys,
         const std::shared_ptr<T3> &zs, Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(bool(zs) == s);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(std::forward<F>(f), *xs, *ys, *zs,
                                         std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F &&, T, Args &&...>>>
R foldMap(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
          Args &&... args) {
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<Op, R, R>>, R>::value, "");
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return R(std::forward<Z>(z));
  return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F &&, T, T2, Args &&...>>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
           const std::shared_ptr<T2> &ys, Args &&... args) {
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<Op, R, R>>, R>::value, "");
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (__builtin_expect(!s, false))
    return R(std::forward<Z>(z));
  return cxx::invoke(std::forward<F>(f), *xs, *ys, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return std::make_shared<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  static_assert(detail::is_shared_ptr<CR>::value, "");
  if (__builtin_expect(!bool(xs), false))
    return CR();
  return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename CT = std::shared_ptr<T>>
CT mjoin(const std::shared_ptr<std::shared_ptr<T>> &xss) {
  if (__builtin_expect(!bool(xss) || !bool(*xss), false))
    return std::shared_ptr<T>();
  return *xss;
}

// mextract

template <typename T> const T &mextract(const std::shared_ptr<T> &xs) {
  assert(bool(xs));
  return *xs;
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename C = std::shared_ptr<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
            Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return std::shared_ptr<R>();
}

// mplus

template <typename T, typename... Ts, typename CT = std::shared_ptr<T>>
CT mplus(const std::shared_ptr<T> &xs, const std::shared_ptr<Ts> &... yss) {
  if (__builtin_expect(bool(xs), true))
    return xs;
  for (auto pys : std::initializer_list<const std::shared_ptr<T> *>{&yss...})
    if (__builtin_expect(bool(*pys), true))
      return *pys;
  return std::shared_ptr<T>();
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR msome(T &&x, Ts &&... ys) {
  return munit<C>(std::forward<T>(x));
}

// mempty

template <typename T> constexpr bool mempty(const std::shared_ptr<T> &xs) {
  return !bool(xs);
}

// msize

template <typename T>
constexpr std::size_t msize(const std::shared_ptr<T> &xs) {
  return !mempty(xs);
}
}

#define FUN_SHARED_PTR_HPP_DONE
#endif // #ifdef FUN_SHARED_PTR_HPP
#ifndef FUN_SHARED_PTR_HPP_DONE
#error "Cyclic include dependency"
#endif
