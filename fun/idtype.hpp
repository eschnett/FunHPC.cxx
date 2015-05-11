#ifndef FUN_IDTYPE_HPP
#define FUN_IDTYPE_HPP

#include <adt/idtype.hpp>
#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_idtype

namespace detail {
template <typename> struct is_idtype : std::false_type {};
template <typename T> struct is_idtype<adt::idtype<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<adt::idtype<T>> {
  template <typename U> using constructor = adt::idtype<U>;
  typedef T value_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_idtype<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == 1);
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                    std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  return adt::idtype<R>(
      cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                                    std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const adt::idtype<T> &xs, const adt::idtype<T2> &ys,
           Args &&... args) {
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                                    std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(
      std::forward<Op>(op), std::forward<Z>(z),
      cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, adt::idtype<T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                     cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                                 std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs,
           const adt::idtype<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                     cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                                 std::forward<Args>(args)...));
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_idtype<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return adt::idtype<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T> auto mjoin(const adt::idtype<adt::idtype<T>> &xss) {
  return xss.get();
}

template <typename T> auto mjoin(adt::idtype<adt::idtype<T>> &&xss) {
  return std::move(xss.get());
}

// mextract

template <typename T> decltype(auto) mextract(const adt::idtype<T> &xs) {
  return xs.get();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs,
              Args &&... args) {
  return munit<adt::idtype>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                                    std::forward<Z>(z), xs,
                                    std::forward<Args>(args)...));
}

// mempty

template <typename T> bool mempty(const adt::idtype<T> &xs) { return false; }
}

#define FUN_IDTYPE_HPP_DONE
#endif // #ifdef FUN_IDTYPE_HPP
#ifndef FUN_IDTYPE_HPP_DONE
#error "Cyclic include dependency"
#endif
