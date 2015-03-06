#ifndef FUN_PAIR_HPP
#define FUN_PAIR_HPP

#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_pair

namespace detail {
template <typename> struct is_pair : std::false_type {};
template <typename L, typename R>
struct is_pair<std::pair<L, R>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, typename L> struct fun_traits<std::pair<L, T>> {
  template <typename U> using constructor = std::pair<L, U>;
  typedef T value_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename L = typename C<R>::first_type,
          std::enable_if_t<detail::is_pair<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == 1);
  return std::pair<L, R>(L(), cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                          std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename L, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const std::pair<L, T> &xs, Args &&... args) {
  return std::pair<L, R>(xs.first, cxx::invoke(std::forward<F>(f), xs.second,
                                               std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::pair<L, T> &&xs, Args &&... args) {
  return std::pair<L, R>(std::move(xs.left),
                         cxx::invoke(std::forward<F>(f), std::move(xs.second),
                                     std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const std::pair<L, T> &xs, const std::pair<L, T2> &ys,
           Args &&... args) {
  return std::pair<L, R>(xs.first,
                         cxx::invoke(std::forward<F>(f), xs.second, ys.second,
                                     std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(
      std::forward<Op>(op), std::forward<Z>(z),
      cxx::invoke(std::forward<F>(f), xs.second, std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, std::pair<L, T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                     cxx::invoke(std::forward<F>(f), std::move(xs.second),
                                 std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs,
           const std::pair<L, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                     cxx::invoke(std::forward<F>(f), xs.second, ys.second,
                                 std::forward<Args>(args)...));
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          typename L = typename C<R>::first_type,
          std::enable_if_t<detail::is_pair<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return std::pair<L, R>(L(), std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename L, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const std::pair<L, T> &xs, Args &&... args) {
  return cxx::invoke(std::forward<F>(f), xs.second,
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename L, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, std::pair<L, T> &&xs, Args &&... args) {
  static_assert(detail::is_pair<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.second),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename L>
auto mjoin(const std::pair<L, std::pair<L, T>> &xss) {
  return xss.second;
}

template <typename T, typename L>
auto mjoin(std::pair<L, std::pair<L, T>> &&xss) {
  return std::move(xss.second);
}

// mextract

template <typename T, typename L>
decltype(auto) mextract(const std::pair<L, T> &xs) {
  return xs.second;
}

template <typename T, typename L>
decltype(auto) mextract(std::pair<L, T> &&xs) {
  return std::move(xs.second);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs,
              Args &&... args) {
  return std::pair<L, R>(
      xs.first, foldMap(std::forward<F>(f), std::forward<Op>(op),
                        std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mempty

template <typename T, typename L> bool mempty(const std::pair<L, T> &xs) {
  return false;
}
}

#define FUN_PAIR_HPP_DONE
#endif // #ifdef FUN_PAIR_HPP
#ifndef FUN_PAIR_HPP_DONE
#error "Cyclic include dependency"
#endif
