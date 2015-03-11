#ifndef FUN_PROXY_HPP
#define FUN_PROXY_HPP

#include <cxx/invoke.hpp>
#include <funhpc/proxy.hpp>

#include <cereal/types/tuple.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fun {

// is_proxy

namespace detail {
template <typename> struct is_proxy : std::false_type {};
template <typename T> struct is_proxy<funhpc::proxy<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<funhpc::proxy<T>> {
  template <typename U> using constructor = funhpc::proxy<U>;
  typedef T value_type;
};

// iotaMap

// TODO: use remote
template <template <typename> class C, typename F, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<
              std::decay_t<F>, std::ptrdiff_t, std::decay_t<Args>...>>,
          std::enable_if_t<detail::is_proxy<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == 1);
  return funhpc::local(std::forward<F>(f), std::ptrdiff_t(0),
                       std::forward<Args>(args)...);
}

// fmap

namespace detail {
template <typename T> struct proxy_fmap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    return cxx::invoke(std::move(f), *xs, std::move(args)...);
  }
};
}

template <typename F, typename T, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>
auto fmap(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  bool s = bool(xs);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap<T>(),
                        std::forward<F>(f), xs, std::forward<Args>(args)...);
}

namespace detail {
template <typename T, typename T2> struct proxy_fmap2 : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    auto ysl = ys.make_local();
    return cxx::invoke(std::move(f), *xs, *ysl, std::move(args)...);
  }
};
}

template <typename F, typename T, typename T2, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>>>
auto fmap2(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
           Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap2<T, T2>(),
                        std::forward<F>(f), xs, ys,
                        std::forward<Args>(args)...);
}

// foldMap

namespace detail {
template <typename T> struct proxy_foldMap : std::tuple<> {
  template <typename F, typename Op, typename Z, typename... Args>
  auto operator()(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
                  Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    return cxx::invoke(
        std::forward<Op>(op), std::forward<Z>(z),
        cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...));
  }
};
}

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>
R foldMap(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::proxy_foldMap<T>(), std::forward<F>(f),
                       std::forward<Op>(op), std::forward<Z>(z), xs,
                       std::forward<Args>(args)...).get();
}

namespace detail {
template <typename T, typename T2> struct proxy_foldMap2 : std::tuple<> {
  template <typename F, typename Op, typename Z, typename... Args>
  auto operator()(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    auto ysl = ys.make_local();
    return cxx::invoke(std::forward<Op>(op), std::forward<Z>(z),
                       cxx::invoke(std::forward<F>(f), *xs, *ysl,
                                   std::forward<Args>(args)...));
  }
};
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args, typename R = std::decay_t<cxx::invoke_of_t<
                                std::decay_t<F>, T, T2, std::decay_t<Args>...>>>
R foldMap2(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
           const funhpc::proxy<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::proxy_foldMap2<T, T2>(), std::forward<F>(f),
                       std::forward<Op>(op), std::forward<Z>(z), xs, ys,
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

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>
auto mbind(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename T> decltype(auto) mextract(const funhpc::proxy<T> &xs) {
  assert(bool(xs));
  return *xs.make_local();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>
auto mfoldMap(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
              Args &&... args) {
  return funhpc::remote(xs.get_proc_future(), detail::proxy_foldMap<T>(),
                        std::forward<F>(f), std::forward<Op>(op),
                        std::forward<Z>(z), xs, std::forward<Args>(args)...);
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_proxy<C<R>>::value> * = nullptr>
auto mzero() {
  return funhpc::proxy<R>();
}

// mempty

template <typename T> bool mempty(const funhpc::proxy<T> &xs) {
  return !bool(xs);
}
}

#define FUN_PROXY_HPP_DONE
#endif // #ifdef FUN_PROXY_HPP
#ifndef FUN_PROXY_HPP_DONE
#error "Cyclic include dependency"
#endif
