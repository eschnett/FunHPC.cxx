#ifndef FUN_EITHER_HPP
#define FUN_EITHER_HPP

#include <adt/either.hpp>
#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_either

namespace detail {
template <typename> struct is_either : std::false_type {};
template <typename L, typename R>
struct is_either<adt::either<L, R>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename L, typename R> struct fun_traits<adt::either<L, R>> {
  template <typename U> using constructor = adt::either<L, U>;
  typedef R value_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename L = typename C<R>::left_type,
          std::enable_if_t<detail::is_either<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s <= 1);
  if (s == 0)
    return adt::either<L, R>();
  return adt::make_right<L, R>(cxx::invoke(
      std::forward<F>(f), std::ptrdiff_t(0), std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename L, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  bool s = xs.right();
  if (!s)
    return adt::either<L, R>();
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                           std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  bool s = xs.right();
  if (!s)
    return adt::either<L, R>();
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f),
                                           std::move(xs.get_right()),
                                           std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const adt::either<L, T> &xs, const adt::either<L, T2> &ys,
           Args &&... args) {
  bool s = xs.right();
  assert(ys.right() == s);
  if (!s)
    return adt::either<L, R>();
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                           ys.get_right(),
                                           std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, const adt::either<L, T> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return z;
  return cxx::invoke(std::forward<Op>(op), z,
                     cxx::invoke(std::forward<F>(f), xs.get_right(),
                                 std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, adt::either<L, T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return z;
  return cxx::invoke(std::forward<Op>(op), z,
                     cxx::invoke(std::forward<F>(f), std::move(xs.get_right()),
                                 std::forward<Args>(args)...));
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, const Z &z, const adt::either<L, T> &xs,
           const adt::either<L, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  assert(ys.right() == s);
  if (!s)
    return z;
  return cxx::invoke(std::forward<Op>(op), z,
                     cxx::invoke(std::forward<F>(f), xs.get_right(),
                                 ys.get_right(), std::forward<Args>(args)...));
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          typename L = typename C<R>::left_type,
          std::enable_if_t<detail::is_either<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return adt::make_right<L, R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename L, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  static_assert(detail::is_either<CR>::value, "");
  if (!xs.right())
    return CR();
  return cxx::invoke(std::forward<F>(f), xs.get_right(),
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename L, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  static_assert(detail::is_either<CR>::value, "");
  if (!xs.right())
    return CR();
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_right()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename L>
auto mjoin(const adt::either<L, adt::either<L, T>> &xss) {
  if (!xss.right() || !xss.get_right().right())
    return adt::either<L, T>();
  return xss.get_right();
}

template <typename T, typename L>
auto mjoin(adt::either<L, adt::either<L, T>> &&xss) {
  if (!xss.right() || !xss.get_right().right())
    return adt::either<L, T>();
  return std::move(xss.get_right());
}

// mextract

template <typename T, typename L>
decltype(auto) mextract(const adt::either<L, T> &xs) {
  assert(xs.right());
  return xs.get_right();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto mfoldMap(F &&f, Op &&op, const Z &z, const adt::either<L, T> &xs,
              Args &&... args) {
  struct S {
    template <typename U> using either1 = adt::either<L, U>;
  };
  return munit<S::template either1>(foldMap(std::forward<F>(f),
                                            std::forward<Op>(op), z, xs,
                                            std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          typename L = typename C<R>::left_type,
          std::enable_if_t<detail::is_either<C<R>>::value> * = nullptr>
auto mzero() {
  return adt::either<L, R>();
}

// mplus

template <typename T, typename L, typename... Ts>
auto mplus(const adt::either<L, T> &xs, const adt::either<L, Ts> &... yss) {
  if (xs.right())
    return xs;
  for (auto pys : std::initializer_list<const adt::either<L, T> *>{&yss...})
    if ((*pys).right())
      return *pys;
  return adt::either<L, T>();
}

template <typename T, typename L, typename... Ts>
auto mplus(adt::either<L, T> &&xs, adt::either<L, Ts> &&... yss) {
  if (xs.right())
    return std::move(xs);
  for (auto pys : std::initializer_list<adt::either<L, T> *>{&yss...})
    if ((*pys).right())
      return std::move(*pys);
  return adt::either<L, T>();
}

// mempty

template <typename T, typename L> bool mempty(const adt::either<L, T> &xs) {
  return !xs.right();
}
}

#define FUN_EITHER_HPP_DONE
#endif // #ifdef FUN_EITHER_HPP
#ifndef FUN_EITHER_HPP_DONE
#error "Cyclic include dependency"
#endif
