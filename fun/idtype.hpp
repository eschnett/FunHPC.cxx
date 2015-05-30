#ifndef FUN_IDTYPE_HPP
#define FUN_IDTYPE_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <adt/empty.hpp>
#include <adt/idtype.hpp>
#include <cxx/invoke.hpp>

#include <algorithm>
#include <cassert>
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
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;
  typedef adt::empty<adt::dummy> boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_idtype<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  assert(inds.size() == 1);
  return adt::idtype<R>(
      cxx::invoke(std::forward<F>(f), inds[0], std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args, typename C = adt::idtype<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  return CR(
      cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args, typename C = adt::idtype<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  return CR(cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                        std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = adt::idtype<T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::idtype<T> &xs, const adt::idtype<T2> &ys,
         Args &&... args) {
  return CR(cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                        std::forward<Args>(args)...));
}

// head, last

template <typename T> constexpr const T &head(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr const T &last(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr T &&head(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

template <typename T> constexpr T &&last(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, adt::idtype<T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs,
           const adt::idtype<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                     std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_idtype<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR munit(T &&x) {
  return CR(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename CT = adt::idtype<T>>
constexpr CT mjoin(const adt::idtype<adt::idtype<T>> &xss) {
  return xss.get();
}

template <typename T, typename CT = adt::idtype<T>>
constexpr CT mjoin(adt::idtype<adt::idtype<T>> &&xss) {
  return std::move(xss.get());
}

// mextract

template <typename T> constexpr const T &mextract(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr T &&mextract(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename C = adt::idtype<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs, Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mempty

template <typename T> constexpr bool mempty(const adt::idtype<T> &xs) {
  return false;
}

// msize

template <typename T> constexpr std::size_t msize(const adt::idtype<T> &xs) {
  return 1;
}
}

#define FUN_IDTYPE_HPP_DONE
#endif // #ifdef FUN_IDTYPE_HPP
#ifndef FUN_IDTYPE_HPP_DONE
#error "Cyclic include dependency"
#endif
