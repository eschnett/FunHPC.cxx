#ifndef FUN_FUNCTION_HPP
#define FUN_FUNCTION_HPP

#include <cxx/invoke.hpp>

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

namespace fun {

// is_function

namespace detail {
// Note: These are functions that take a single argument
template <typename> struct is_function : std::false_type {};
template <typename R, typename A>
struct is_function<std::function<R(A)>> : std::true_type {};

template <typename A> struct function1 {
  template <typename R> using constructor = std::function<R(A)>;
};
}

// iota

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          std::enable_if_t<detail::is_function<C<R>>::value> * = nullptr>
auto iota(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef typename C<R>::argument_type A;
  assert(s <= 1);
  if (s == 0)
    return C<R>();
  C<R> rs = [ f = std::forward<F>(f), args... ](const A &) {
    return cxx::invoke(f, std::ptrdiff_t(0), args...);
  };
  return rs;
}

// fmap

template <
    typename F, typename T, typename A, typename... Args,
    typename R = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>,
    template <typename> class C = detail::function1<A>::template constructor>
auto fmap(F &&f, const std::function<T(A)> &xs, Args &&... args) {
  bool s = bool(xs);
  if (!s)
    return C<R>();
  C<R> rs = [ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(f, cxx::invoke(xs, a), args...);
  };
  return rs;
}

template <typename F, typename T, typename A, typename T2, typename... Args,
          typename R =
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>,
          template <typename>
          class C = detail::function1<A>::template constructor>
auto fmap2(F &&f, const std::function<T(A)> &xs, const std::function<T2(A)> &ys,
           Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (!s)
    return C<R>();
  C<R> rs = [ f = std::forward<F>(f), xs, ys, args... ](const A &a) {
    return cxx::invoke(f, cxx::invoke(xs, a), cxx::invoke(ys, a), args...);
  };
  return rs;
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_function<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  typedef typename C<R>::argument_type A;
  C<R> rs = [x = std::forward<T>(x)](const A &) { return x; };
  return rs;
}

// mbind
// (a -> (x -> b)) -> (x -> a) -> (x -> b)

template <
    typename F, typename T, typename A, typename... Args,
    typename CR = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
auto mbind(F &&f, const std::function<T(A)> &xs, Args &&... args) {
  static_assert(detail::is_function<CR>::value, "");
  if (!bool(xs))
    return CR();
  CR rs = [ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(cxx::invoke(f, cxx::invoke(xs, a), args...), a);
  };
  return rs;
}

// mjoin
// (x -> (x -> a)) -> (x -> a)

template <typename T, typename A>
auto mjoin(const std::function<std::function<T(A)>(A)> &xss) {
  if (!bool(xss))
    return std::function<T(A)>();
  std::function<T(A)> rs =
      [xss](const A &a) { return cxx::invoke(cxx::invoke(xss, a), a); };
  return rs;
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_function<C<R>>::value> * = nullptr>
auto mzero() {
  return C<R>();
}

// mplus

template <typename T, typename A, typename... Ts>
auto mplus(const std::function<T(A)> &xs, const std::function<Ts(A)> &... yss) {
  if (bool(xs))
    return xs;
  for (auto pys : std::initializer_list<const std::function<T(A)> *>{&yss...})
    if (bool(*pys))
      return *pys;
  return std::function<T(A)>();
}

// mempty

template <typename T> bool mempty(const std::function<T> &xs) {
  return !bool(xs);
}
}

#define FUN_FUNCTION_HPP_DONE
#endif // #ifdef FUN_FUNCTION_HPP
#ifndef FUN_FUNCTION_HPP_DONE
#error "Cyclic include dependency"
#endif
