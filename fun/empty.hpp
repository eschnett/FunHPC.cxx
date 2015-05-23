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
          std::enable_if_t<detail::is_empty<C>::value> * = nullptr>
constexpr auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  assert(s == 0);
  return adt::empty<R>();
}

// fmap

template <typename F, typename T, typename... Args>
constexpr auto fmap(F &&f, const adt::empty<T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  return adt::empty<R>();
}

template <typename F, typename T, typename T2, typename... Args>
constexpr auto fmap2(F &&f, const adt::empty<T> &xs, const adt::empty<T2> &ys,
                     Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  return adt::empty<R>();
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args>
constexpr auto foldMap(F &&f, Op &&op, Z &&z, const adt::empty<T> &xs,
                       Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args>
constexpr auto foldMap2(F &&f, Op &&op, Z &&z, const adt::empty<T> &xs,
                        const adt::empty<T2> &ys, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, T2, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

// mjoin

template <typename T>
constexpr auto mjoin(const adt::empty<adt::empty<T>> &xss) {
  return adt::empty<T>();
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
