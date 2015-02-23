#ifndef FUN_PROXY_HPP
#define FUN_PROXY_HPP

#include <cxx/invoke.hpp>
#include <funhpc/proxy.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_proxy

namespace detail {
template <typename> struct is_proxy : std::false_type {};
template <typename T> struct is_proxy<funhpc::proxy<T>> : std::true_type {};
}

// iota

// TODO: use remote
template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          std::enable_if_t<detail::is_proxy<C<R>>::value> * = nullptr>
auto iota(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == 1);
  return funhpc::local(std::forward<F>(f), std::ptrdiff_t(0),
                       std::forward<Args>(args)...);
}

// fmap

namespace detail {
template <
    typename F, typename T, typename... Args,
    typename R = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
auto fmap_local(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  assert(bool(xs) && xs.proc_ready() && xs.local());
  return cxx::invoke(std::move(f), *xs, std::move(args)...);
}
}

template <
    typename F, typename T, typename... Args,
    typename R = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
auto fmap(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  bool s = bool(xs);
  assert(s);
  return funhpc::remote(
      xs.get_proc_future(),
      detail::fmap_local<std::decay_t<F>, T, std::decay_t<Args>...>,
      std::forward<F>(f), xs, std::forward<Args>(args)...);
}

namespace detail {
template <typename F, typename T, typename T2, typename... Args,
          typename R =
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>>
auto fmap2_local(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
                 Args &&... args) {
  assert(bool(xs) && xs.proc_ready() && xs.local());
  assert(bool(ys));
  auto ysl = ys.make_local();
  return cxx::invoke(std::move(f), *xs, *ysl, std::move(args)...);
}
}

template <typename F, typename T, typename T2, typename... Args,
          typename R =
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>>
auto fmap2(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
           Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::remote(
      xs.get_proc_future(),
      detail::fmap2_local<std::decay_t<F>, T, T2, std::decay_t<Args>...>,
      std::forward<F>(f), xs, ys, std::forward<Args>(args)...);
}

// foldMap

namespace detail {
template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap_local(F &&f, Op &&op, const Z &z, const funhpc::proxy<T> &xs,
                Args &&... args) {
  assert(bool(xs) && xs.proc_ready() && xs.local());
  return cxx::invoke(std::move(op), z,
                     cxx::invoke(std::move(f), *xs, std::move(args)...));
}
}

template <
    typename F, typename Op, typename Z, typename T, typename... Args,
    typename R = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
R foldMap(F &&f, Op &&op, const Z &z, const funhpc::proxy<T> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::foldMap_local<std::decay_t<F>, std::decay_t<Op>,
                                             Z, T, std::decay_t<Args>...>,
                       std::forward<F>(f), std::forward<Op>(op), z, xs,
                       std::forward<Args>(args)...).get();
}

namespace detail {
template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2_local(F &&f, Op &&op, const Z &z, const funhpc::proxy<T> &xs,
                 const funhpc::proxy<T2> &ys, Args &&... args) {
  assert(bool(xs) && xs.proc_ready() && xs.local());
  assert(bool(ys));
  auto ysl = ys.make_local();
  return cxx::invoke(std::move(op), z,
                     cxx::invoke(std::move(f), *xs, *ysl, std::move(args)...));
}
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<
                                std::decay_t<F>, T, T2, std::decay_t<Args>...>>
R foldMap2(F &&f, Op &&op, const Z &z, const funhpc::proxy<T> &xs,
           const funhpc::proxy<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::foldMap2_local<std::decay_t<F>, std::decay_t<Op>,
                                              Z, T, T2, std::decay_t<Args>...>,
                       std::forward<F>(f), std::forward<Op>(op), z, xs, ys,
                       std::forward<Args>(args)...).get();
}

// munit

// TODO: use make_remote_proxy
template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_proxy<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return funhpc::make_local_proxy<R>(std::forward<T>(x));
}

// mjoin

template <typename T> auto mjoin(const funhpc::proxy<funhpc::proxy<T>> &xss) {
  assert(bool(xss));
  return xss.unwrap();
}

// mbind

// IMPLEMENT VIA MJOIN
template <
    typename F, typename T, typename... Args,
    typename CR = cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>
auto mbind(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  static_assert(detail::is_proxy<CR>::value, "");
  assert(bool(xs));
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

// mextract

template <typename T> decltype(auto) mextract(const funhpc::proxy<T> &xs) {
  assert(xs.valid());
  return xs.get();
}

template <typename T> decltype(auto) mextract(funhpc::proxy<T> &&xs) {
  assert(xs.valid());
  return std::move(xs.get());
}
}

#define FUN_PROXY_HPP_DONE
#endif // #ifdef FUN_PROXY_HPP
#ifndef FUN_PROXY_HPP_DONE
#error "Cyclic include dependency"
#endif
