#ifndef FUN_SHARED_PTR_HPP
#define FUN_SHARED_PTR_HPP

#include <adt/dummy.hpp>
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
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>> R;
  assert(s <= 1);
  if (__builtin_expect(s == 0, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                         std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args>
auto fmap(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F, T, Args...>> R;
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args>
auto fmap2(F &&f, const std::shared_ptr<T> &xs, const std::shared_ptr<T2> &ys,
           Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F, T, T2, Args...>> R;
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (__builtin_expect(!s, false))
    return std::shared_ptr<R>();
  return std::make_shared<R>(
      cxx::invoke(std::forward<F>(f), *xs, *ys, std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
             Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F &&, T, Args &&...>> R;
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<Op, R, R>>, R>::value, "");
  bool s = bool(xs);
  if (__builtin_expect(!s, false))
    return R(std::forward<Z>(z));
  return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
              const std::shared_ptr<T2> &ys, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F &&, T, T2, Args &&...>> R;
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
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef std::decay_t<T> R;
  return std::make_shared<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args>
auto mbind(F &&f, const std::shared_ptr<T> &xs, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<F, T, Args...>> CR;
  static_assert(detail::is_shared_ptr<CR>::value, "");
  if (__builtin_expect(!bool(xs), false))
    return CR();
  return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
}

// mjoin

template <typename T>
auto mjoin(const std::shared_ptr<std::shared_ptr<T>> &xss) {
  if (__builtin_expect(!bool(xss) || !bool(*xss), false))
    return std::shared_ptr<T>();
  return *xss;
}

// mextract

template <typename T> decltype(auto) mextract(const std::shared_ptr<T> &xs) {
  assert(bool(xs));
  return *xs;
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::shared_ptr<T> &xs,
              Args &&... args) {
  typedef typename fun_traits<std::shared_ptr<T>>::dummy C;
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr>
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

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_shared_ptr<C>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
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
