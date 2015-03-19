#ifndef FUN_SHARED_PTR_HPP
#define FUN_SHARED_PTR_HPP

#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
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
  template <typename U> using constructor = std::shared_ptr<U>;
  typedef T value_type;
};

// iotaMap

template <
    template <typename> class C, typename F, typename... Args,
    typename R = std::decay_t<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>>,
    std::enable_if_t<detail::is_shared_ptr<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s <= 1);
  if (__builtin_expect(s == 0, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                         std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
auto fmap(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
auto fmap(F &&f, std::shared_ptr<T> &&xs, Args &&... args) {
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(std::forward<F>(f), std::move(*xs),
                                         std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F, T, T2, Args...>>>
auto fmap2(F &&f, const std::shared_ptr<T> &xs, const std::shared_ptr<T2> &ys,
           Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, *ys, std::forward<Args>(args)...));
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
    return std::forward<Z>(z);
  return cxx::invoke(
      std::forward<Op>(op), std::forward<Z>(z),
      cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F &&, T, Args &&...>>>
R foldMap(F &&f, Op &&op, Z &&z, std::shared_ptr<T> &&xs, Args &&... args) {
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<Op, R, R>>, R>::value, "");
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                     cxx::invoke(std::forward<F>(f), std::move(*xs),
                                 std::forward<Args>(args)...));
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
    return std::forward<Z>(z);
  return cxx::invoke(
      std::forward<Op>(op), std::forward<Z>(z),
      cxx::invoke(std::forward<F>(f), *xs, *ys, std::forward<Args>(args)...));
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_shared_ptr<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return std::make_shared<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
auto mbind(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  static_assert(detail::is_shared_ptr<CR>::value, "");
  if (__builtin_expect(!bool(xs), false))
    return CR();
  return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
auto mbind(F &&f, std::shared_ptr<T> &&xs, Args &&... args) {
  static_assert(detail::is_shared_ptr<CR>::value, "");
  if (__builtin_expect(!bool(xs), false))
    return CR();
  return cxx::invoke(std::forward<F>(f), std::move(*xs),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T>
auto mjoin(const std::shared_ptr<std::shared_ptr<T>> &xss) {
  if (__builtin_expect(!bool(xss) || !bool(*xss), false))
    return std::shared_ptr<T>();
  return *xss;
}

template <typename T> auto mjoin(std::shared_ptr<std::shared_ptr<T>> &&xss) {
  if (__builtin_expect(!bool(xss) || !bool(*xss), false))
    return std::shared_ptr<T>();
  return std::move(*xss);
}

// mextract

template <typename T> decltype(auto) mextract(const std::shared_ptr<T> &xs) {
  assert(bool(xs));
  return *xs;
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<F &&, T, Args &&...>>>
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
              Args &&... args) {
  return munit<std::shared_ptr>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_shared_ptr<C<R>>::value> * = nullptr>
auto mzero() {
  return std::shared_ptr<R>();
}

// mplus

template <typename T, typename... Ts>
auto mplus(const std::shared_ptr<T> &xs, const std::shared_ptr<Ts> &... yss) {
  if (__builtin_expect(bool(xs), true))
    return xs;
  for (auto pys : std::initializer_list<const std::shared_ptr<T> *>{&yss...})
    if (__builtin_expect(bool(*pys), true))
      return *pys;
  return std::shared_ptr<T>();
}

template <typename T, typename... Ts>
auto mplus(std::shared_ptr<T> &&xs, std::shared_ptr<Ts> &&... yss) {
  if (__builtin_expect(bool(xs), true))
    return std::move(xs);
  for (auto pys : std::initializer_list<std::shared_ptr<T> *>{&yss...})
    if (__builtin_expect(bool(*pys), true))
      return std::move(*pys);
  return std::shared_ptr<T>();
}

// msome

template <template <typename> class C, typename T, typename... Ts,
          typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_shared_ptr<C<R>>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
  return munit<C>(std::forward<T>(x));
}

// mempty

template <typename T> bool mempty(const std::shared_ptr<T> &xs) {
  return !bool(xs);
}
}

#define FUN_SHARED_PTR_HPP_DONE
#endif // #ifdef FUN_SHARED_PTR_HPP
#ifndef FUN_SHARED_PTR_HPP_DONE
#error "Cyclic include dependency"
#endif
