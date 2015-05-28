#ifndef FUN_EMPTY_HPP
#define FUN_EMPTY_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <adt/empty.hpp>
#include <cxx/invoke.hpp>

#include <cassert>
#include <type_traits>

namespace fun {

// is_empty

namespace detail {
template <typename> struct is_empty : std::false_type {};
template <typename T> struct is_empty<adt::empty<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<adt::empty<T>> {
  template <typename U> using constructor = adt::empty<U>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;
  typedef adt::empty<adt::dummy> boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_empty<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == 0);
  return CR();
}

// fmap

template <typename F, typename T, typename... Args, typename C = adt::empty<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR fmap(F &&f, const adt::empty<T> &xs, Args &&... args) {
  return CR();
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = adt::empty<T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR fmap2(F &&f, const adt::empty<T> &xs, const adt::empty<T2> &ys,
                   Args &&... args) {
  return CR();
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
constexpr R foldMap(F &&f, Op &&op, Z &&z, const adt::empty<T> &xs,
                    Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
constexpr R foldMap2(F &&f, Op &&op, Z &&z, const adt::empty<T> &xs,
                     const adt::empty<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
constexpr CR mbind(F &&f, const adt::empty<T> &xs, Args &&... args) {
  static_assert(detail::is_empty<CR>::value, "");
  return CR();
}

// mjoin

template <typename T, typename CT = adt::empty<T>>
constexpr CT mjoin(const adt::empty<adt::empty<T>> &xss) {
  return CT();
}

// mempty

template <typename T> constexpr bool mempty(const adt::empty<T> &xs) {
  return true;
}

// msize

template <typename T> constexpr std::size_t msize(const adt::empty<T> &xs) {
  return 0;
}
}

#define FUN_EMPTY_HPP_DONE
#endif // #ifdef FUN_EMPTY_HPP
#ifndef FUN_EMPTY_HPP_DONE
#error "Cyclic include dependency"
#endif
