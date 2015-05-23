#ifndef FUN_FUNCTION_HPP
#define FUN_FUNCTION_HPP

#include <adt/dummy.hpp>
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
}

// traits

template <typename> struct fun_traits;
template <typename R, typename A> struct fun_traits<std::function<R(A)>> {
  template <typename U> using constructor = std::function<U(A)>;
  typedef constructor<adt::dummy> dummy;
  typedef R value_type;

  // typedef A argument_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  typedef typename C::argument_type A;
  assert(s <= 1);
  if (s == 0)
    return CR();
  // TODO: Make this serializable
  CR rs = [ f = std::forward<F>(f), args... ](const A &) {
    return cxx::invoke(f, std::ptrdiff_t(0), args...);
  };
  return rs;
}

// fmap

template <typename F, typename T, typename A, typename... Args>
auto fmap(F &&f, const std::function<T(A)> &xs, Args &&... args) {
  typedef typename fun_traits<std::function<T(A)>>::dummy C;
  typedef cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  bool s = bool(xs);
  if (!s)
    return CR();
  CR rs = [ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(f, cxx::invoke(xs, a), args...);
  };
  return rs;
}

template <typename F, typename T, typename A, typename T2, typename... Args>
auto fmap2(F &&f, const std::function<T(A)> &xs, const std::function<T2(A)> &ys,
           Args &&... args) {
  typedef typename fun_traits<std::function<T(A)>>::dummy C;
  typedef cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  bool s = bool(xs);
  assert(bool(ys) == s);
  if (!s)
    return CR();
  CR rs = [ f = std::forward<F>(f), xs, ys, args... ](const A &a) {
    return cxx::invoke(f, cxx::invoke(xs, a), cxx::invoke(ys, a), args...);
  };
  return rs;
}

// munit

template <typename C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef typename fun_traits<C>::template constructor<R> CR;
  typedef typename C::argument_type A;
  CR rs = [x = std::forward<T>(x)](const A &) { return x; };
  return rs;
}

// mbind
// (a -> (x -> b)) -> (x -> a) -> (x -> b)

template <typename F, typename T, typename A, typename... Args>
auto mbind(F &&f, const std::function<T(A)> &xs, Args &&... args) {
  typedef cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...> CR;
  static_assert(detail::is_function<CR>::value, "");
  if (!bool(xs))
    return CR();
  // TODO: allow empty f?
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

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename A,
          typename... Args>
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::function<T(A)> &xs,
              Args &&... args) {
  typedef typename fun_traits<std::function<T(A)>>::dummy C;
  typedef cxx::invoke_of_t<F &&, T, Args &&...> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR([ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(f, xs(a), args...);
  });
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr>
auto mzero() {
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR();
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

// msize

template <typename T> std::size_t msize(const std::function<T> &xs) {
  return !mempty(xs);
}
}

#define FUN_FUNCTION_HPP_DONE
#endif // #ifdef FUN_FUNCTION_HPP
#ifndef FUN_FUNCTION_HPP_DONE
#error "Cyclic include dependency"
#endif
