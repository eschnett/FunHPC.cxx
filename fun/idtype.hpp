#ifndef FUN_IDTYPE_HPP
#define FUN_IDTYPE_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <adt/empty.hpp>
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
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;
  typedef adt::empty<adt::dummy> boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_idtype<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  assert(s == 1);
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), std::ptrdiff_t(0),
                                    std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename... Args>
auto fmap(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  return adt::idtype<R>(
      cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args>
auto fmap(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                                    std::forward<Args>(args)...));
}

template <typename F, typename T, typename T2, typename... Args>
auto fmap2(F &&f, const adt::idtype<T> &xs, const adt::idtype<T2> &ys,
           Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  return adt::idtype<R>(cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                                    std::forward<Args>(args)...));
}

// head, last

template <typename T> constexpr decltype(auto) head(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr decltype(auto) last(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr decltype(auto) head(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

template <typename T> constexpr decltype(auto) last(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, adt::idtype<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs,
              const adt::idtype<T2> &ys, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, T2, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                     std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_idtype<C>::value> * = nullptr>
constexpr auto munit(T &&x) {
  typedef std::decay_t<T> R;
  return adt::idtype<R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename... Args>
auto mbind(F &&f, const adt::idtype<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args>
auto mbind(F &&f, adt::idtype<T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
  static_assert(detail::is_idtype<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T>
constexpr auto mjoin(const adt::idtype<adt::idtype<T>> &xss) {
  return xss.get();
}

template <typename T> constexpr auto mjoin(adt::idtype<adt::idtype<T>> &&xss) {
  return std::move(xss.get());
}

// mextract

template <typename T>
constexpr decltype(auto) mextract(const adt::idtype<T> &xs) {
  return xs.get();
}

template <typename T> constexpr decltype(auto) mextract(adt::idtype<T> &&xs) {
  return std::move(xs.get());
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::idtype<T> &xs,
              Args &&... args) {
  typedef typename fun_traits<adt::idtype<T>>::dummy C;
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
