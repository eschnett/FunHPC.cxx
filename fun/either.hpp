#ifndef FUN_EITHER_HPP
#define FUN_EITHER_HPP

#include <adt/dummy.hpp>
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
  typedef constructor<adt::dummy> dummy;
  typedef R value_type;

  typedef L left_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  typedef typename C::left_type L;
  assert(s <= 1);
  if (s == 0)
    return adt::either<L, R>();
  return adt::make_right<L, R>(cxx::invoke(
      std::forward<F>(f), std::ptrdiff_t(0), std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename L, typename... Args>
auto fmap(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  bool s = xs.right();
  if (!s)
    return adt::make_left<L, R>(xs.get_left());
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                           std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename... Args>
auto fmap(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> R;
  bool s = xs.right();
  if (!s)
    return adt::make_left<L, R>(xs.get_left());
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f),
                                           std::move(xs.get_right()),
                                           std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename T2, typename L2,
          typename... Args>
auto fmap2(F &&f, const adt::either<L, T> &xs, const adt::either<L2, T2> &ys,
           Args &&... args) {
  typedef cxx::invoke_of_t<F, T, T2, Args...> R;
  bool s = xs.right();
  assert(ys.right() == s);
  if (!s)
    return adt::make_left<L, R>(xs.get_left());
  return adt::make_right<L, R>(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                           ys.get_right(),
                                           std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs,
             Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_right(),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args>
auto foldMap(F &&f, Op &&op, Z &&z, adt::either<L, T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_right()),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename T2, typename... Args>
auto foldMap2(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs,
              const adt::either<L, T2> &ys, Args &&... args) {
  typedef cxx::invoke_of_t<F &&, T, T2, Args &&...> R;
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  assert(ys.right() == s);
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_right(), ys.get_right(),
                     std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef std::decay_t<T> R;
  typedef typename C::left_type L;
  return adt::make_right<L, R>(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename L, typename... Args>
auto mbind(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
  static_assert(detail::is_either<CR>::value, "");
  if (!xs.right())
    return CR();
  return cxx::invoke(std::forward<F>(f), xs.get_right(),
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename L, typename... Args>
auto mbind(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  typedef cxx::invoke_of_t<F, T, Args...> CR;
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
          typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs,
              Args &&... args) {
  typedef typename fun_traits<adt::either<L, T>>::dummy C;
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr>
auto mzero() {
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR();
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

// msize

template <typename T, typename L>
std::size_t msize(const adt::either<L, T> &xs) {
  return !mempty(xs);
}
}

#define FUN_EITHER_HPP_DONE
#endif // #ifdef FUN_EITHER_HPP
#ifndef FUN_EITHER_HPP_DONE
#error "Cyclic include dependency"
#endif
