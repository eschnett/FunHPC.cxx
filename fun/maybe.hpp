#ifndef FUN_MAYBE_HPP
#define FUN_MAYBE_HPP

#include <adt/dummy.hpp>
#include <adt/maybe.hpp>
#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_maybe

namespace detail {
template <typename> struct is_maybe : std::false_type {};
template <typename T> struct is_maybe<adt::maybe<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<adt::maybe<T>> {
  template <typename U> using constructor = adt::maybe<U>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_maybe<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  assert(s <= 1);
  if (s == 0)
    return adt::maybe<R>();
  return adt::make_just<R>(cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                       std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args>
auto fmap(F &&f, const adt::maybe<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  bool s = xs.just();
  if (!s)
    return adt::maybe<R>();
  return adt::make_just<R>(cxx::invoke(std::forward<F>(f), xs.get_just(),
                                       std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args>
auto fmap(F &&f, adt::maybe<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  bool s = xs.just();
  if (!s)
    return adt::maybe<R>();
  return adt::make_just<R>(cxx::invoke(std::forward<F>(f),
                                       std::move(xs.get_just()),
                                       std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args>
auto fmap2(F &&f, const adt::maybe<T> &xs, const adt::maybe<T2> &ys,
           Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  bool s = xs.just();
  assert(ys.just() == s);
  if (!s)
    return adt::maybe<R>();
  return adt::make_just<R>(cxx::invoke(std::forward<F>(f), xs.get_just(),
                                       ys.get_just(),
                                       std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::maybe<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.just();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_just(),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, adt::maybe<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.just();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_just()),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::maybe<T> &xs,
              const adt::maybe<T2> &ys, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, T2, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.just();
  assert(ys.just() == s);
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_just(), ys.get_just(),
                     std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_maybe<C>::value> * = nullptr>
constexpr auto munit(T &&x) {
  typedef std::decay_t<T> R;
  return adt::make_just<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args>
auto mbind(F &&f, const adt::maybe<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
  static_assert(detail::is_maybe<CR>::value, "");
  if (!xs.just())
    return CR();
  return cxx::invoke(std::forward<F>(f), xs.get_just(),
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args>
auto mbind(F &&f, adt::maybe<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
  static_assert(detail::is_maybe<CR>::value, "");
  if (!xs.just())
    return CR();
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_just()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T> auto mjoin(const adt::maybe<adt::maybe<T>> &xss) {
  if (!xss.just() || !xss.get_just().just())
    return adt::maybe<T>();
  return xss.get_just();
}

template <typename T> auto mjoin(adt::maybe<adt::maybe<T>> &&xss) {
  if (!xss.just() || !xss.get_just().just())
    return adt::maybe<T>();
  return std::move(xss.get_just());
}

// mextract

template <typename T>
constexpr decltype(auto) mextract(const adt::maybe<T> &xs) {
  assert(xs.just());
  return xs.get_just();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::maybe<T> &xs, Args &&... args) {
  typedef typename fun_traits<adt::maybe<T>>::dummy C;
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_maybe<C>::value> * = nullptr>
constexpr auto mzero() {
  return adt::maybe<R>();
}

// mplus

template <typename T, typename... Ts>
constexpr auto mplus(const adt::maybe<T> &xs, const adt::maybe<Ts> &... yss) {
  if (xs.just())
    return xs;
  for (auto pys : std::initializer_list<const adt::maybe<T> *>{&yss...})
    if ((*pys).just())
      return *pys;
  return adt::maybe<T>();
}

template <typename T, typename... Ts>
constexpr auto mplus(adt::maybe<T> &&xs, adt::maybe<Ts> &&... yss) {
  if (xs.just())
    return std::move(xs);
  for (auto pys : std::initializer_list<adt::maybe<T> *>{&yss...})
    if ((*pys).just())
      return std::move(*pys);
  return adt::maybe<T>();
}

// mempty

template <typename T> constexpr bool mempty(const adt::maybe<T> &xs) {
  return !xs.just();
}

// msize

template <typename T> constexpr std::size_t msize(const adt::maybe<T> &xs) {
  return !mempty(xs);
}
}

#define FUN_MAYBE_HPP_DONE
#endif // #ifdef FUN_MAYBE_HPP
#ifndef FUN_MAYBE_HPP_DONE
#error "Cyclic include dependency"
#endif
