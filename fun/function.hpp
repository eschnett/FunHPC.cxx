#ifndef FUN_FUNCTION_HPP
#define FUN_FUNCTION_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <algorithm>
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
  template <typename U> using constructor = std::function<std::decay_t<U>(A)>;
  typedef constructor<adt::dummy> dummy;
  typedef R value_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  typedef typename C::argument_type A;
  assert(inds.size() <= 1);
  if (inds.empty())
    return CR();
  // TODO: Make this serializable
  CR rs = [ f = std::forward<F>(f), i = inds[0], args... ](const A &) {
    return cxx::invoke(f, i, args...);
  };
  return rs;
}

// fmap

template <
    typename F, typename T, typename A, typename... Args,
    typename C = std::function<T(A)>,
    typename R = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const std::function<T(A)> &xs, Args &&... args) {
  bool s = bool(xs);
  if (!s)
    return CR();
  CR rs = [ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(f, cxx::invoke(xs, a), args...);
  };
  return rs;
}

template <typename F, typename T, typename A, typename T2, typename... Args,
          typename C = std::function<T(A)>,
          typename R =
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const std::function<T(A)> &xs, const std::function<T2(A)> &ys,
         Args &&... args) {
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

template <typename C, typename T,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  typedef typename C::argument_type A;
  CR rs = [x = std::forward<T>(x)](const A &) { return x; };
  return rs;
}

// mbind
// (a -> (x -> b)) -> (x -> a) -> (x -> b)

template <
    typename F, typename T, typename A, typename... Args,
    typename CR = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
CR mbind(F &&f, const std::function<T(A)> &xs, Args &&... args) {
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

template <typename T, typename A, typename CT = std::function<T(A)>>
CT mjoin(const std::function<std::function<T(A)>(A)> &xss) {
  if (!bool(xss))
    return CT();
  CT rs = [xss](const A &a) { return cxx::invoke(cxx::invoke(xss, a), a); };
  return rs;
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename A,
          typename... Args, typename C = std::function<T(A)>,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const std::function<T(A)> &xs,
            Args &&... args) {
  return CR([ f = std::forward<F>(f), xs, args... ](const A &a) {
    return cxx::invoke(f, xs(a), args...);
  });
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_function<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return CR();
}

// mplus

template <typename T, typename A, typename... Ts,
          typename CT = std::function<T(A)>>
CT mplus(const std::function<T(A)> &xs, const std::function<Ts(A)> &... yss) {
  if (bool(xs))
    return xs;
  for (auto pys : std::initializer_list<const std::function<T(A)> *>{&yss...})
    if (bool(*pys))
      return *pys;
  return CT();
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
