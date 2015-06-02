#ifndef FUN_PROXY_HPP
#define FUN_PROXY_HPP

#include <funhpc/proxy.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <cereal/types/tuple.hpp>

#include <algorithm>
#include <cassert>
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
  template <typename U> using constructor = funhpc::proxy<std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_proxy<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  assert(inds.size() == 1);
  // TODO: use funhpc::remote
  return funhpc::local(std::forward<F>(f), inds[0],
                       std::forward<Args>(args)...);
}

// fmap

namespace detail {
struct proxy_fmap : std::tuple<> {
  template <typename F, typename T, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename T, typename... Args,
          typename C = funhpc::proxy<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  bool s = bool(xs);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap(),
                        std::forward<F>(f), xs, std::forward<Args>(args)...);
}

namespace detail {
struct proxy_fmap2 : std::tuple<> {
  template <typename F, typename T, typename T2, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    auto ysl = ys.make_local();
    return cxx::invoke(std::forward<F>(f), *xs, *ysl,
                       std::forward<Args>(args)...);
  }
};
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = funhpc::proxy<T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
         Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap2(),
                        std::forward<F>(f), xs, ys,
                        std::forward<Args>(args)...);
}

namespace detail {
struct proxy_fmap3 : std::tuple<> {
  template <typename F, typename T, typename T2, typename T3, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, const funhpc::proxy<T3> &zs,
                  Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    assert(bool(zs));
    auto ysl = ys.make_local();
    auto zsl = zs.make_local();
    return cxx::invoke(std::forward<F>(f), *xs, *ysl, *zsl,
                       std::forward<Args>(args)...);
  }
};
}

template <typename F, typename T, typename T2, typename T3, typename... Args,
          typename C = funhpc::proxy<T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
         const funhpc::proxy<T3> &zs, Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(bool(zs) == s);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap3(),
                        std::forward<F>(f), xs, ys, zs,
                        std::forward<Args>(args)...);
}

namespace detail {
struct proxy_fmap5 : std::tuple<> {
  template <typename F, typename T, typename T2, typename T3, typename T4,
            typename T5, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, const funhpc::proxy<T3> &zs,
                  const funhpc::proxy<T4> &as, const funhpc::proxy<T5> &bs,
                  Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    assert(bool(zs));
    assert(bool(as));
    assert(bool(bs));
    auto ysl = ys.make_local();
    auto zsl = zs.make_local();
    auto asl = as.make_local();
    auto bsl = bs.make_local();
    return cxx::invoke(std::forward<F>(f), *xs, *ysl, *zsl, *asl, *bsl,
                       std::forward<Args>(args)...);
  }
};
}

// TODO: Introduce fmapSome (?), where one can indicate in some way
// which arguments are mapped over. Pass a tied tuple? Use wrappers to
// indicate mapping, or to indicate not mapping?

template <typename F, typename T, typename T2, typename T3, typename T4,
          typename T5, typename... Args, typename C = funhpc::proxy<T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, T4, T5, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap5(F &&f, const funhpc::proxy<T> &xs, const funhpc::proxy<T2> &ys,
         const funhpc::proxy<T3> &zs, const funhpc::proxy<T4> &as,
         const funhpc::proxy<T5> &bs, Args &&... args) {
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(bool(zs) == s);
  assert(bool(as) == s);
  assert(bool(bs) == s);
  assert(s);
  return funhpc::remote(xs.get_proc_future(), detail::proxy_fmap5(),
                        std::forward<F>(f), xs, ys, zs, as, bs,
                        std::forward<Args>(args)...);
}

// foldMap

namespace detail {
struct proxy_foldMap : std::tuple<> {
  template <typename F, typename T, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    return cxx::invoke(std::forward<F>(f), *xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::proxy_foldMap(), std::forward<F>(f), xs,
                       std::forward<Args>(args)...).get();
}

namespace detail {
struct proxy_foldMap2 : std::tuple<> {
  template <typename F, typename T, typename T2, typename... Args>
  auto operator()(F &&f, const funhpc::proxy<T> &xs,
                  const funhpc::proxy<T2> &ys, Args &&... args) const {
    assert(bool(xs) && xs.proc_ready() && xs.local());
    assert(bool(ys));
    auto ysl = ys.make_local();
    return cxx::invoke(std::forward<F>(f), *xs, *ysl,
                       std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
           const funhpc::proxy<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = bool(xs);
  assert(bool(ys) == s);
  assert(s);
  return funhpc::async(funhpc::rlaunch::sync, xs.get_proc_future(),
                       detail::proxy_foldMap2(), std::forward<F>(f), xs, ys,
                       std::forward<Args>(args)...).get();
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_proxy<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x) {
  // TODO: use make_remote_proxy
  return funhpc::make_local_proxy<std::decay_t<T>>(std::forward<T>(x));
}

// mjoin

template <typename T, typename CT = funhpc::proxy<T>>
CT mjoin(const funhpc::proxy<funhpc::proxy<T>> &xss) {
  assert(bool(xss));
  return xss.unwrap();
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const funhpc::proxy<T> &xs, Args &&... args) {
  static_assert(detail::is_proxy<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

// Note: Don't return a reference to a temporary -- cannot use
// decltype(auto)!
template <typename T> T mextract(const funhpc::proxy<T> &xs) {
  assert(bool(xs));
  return *xs.make_local();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename C = funhpc::proxy<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const funhpc::proxy<T> &xs,
            Args &&... args) {
  if (!bool(xs))
    return munit<C>(R(std::forward<Z>(z)));
  return fmap(std::forward<F>(f), xs, std::forward<Args>(args)...);
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_proxy<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR mzero() {
  return funhpc::proxy<R>();
}

// mempty

template <typename T> constexpr bool mempty(const funhpc::proxy<T> &xs) {
  return !bool(xs);
}

// msize

template <typename T> constexpr std::size_t msize(const funhpc::proxy<T> &xs) {
  return !mempty(xs);
}
}

#define FUN_PROXY_HPP_DONE
#endif // #ifdef FUN_PROXY_HPP
#ifndef FUN_PROXY_HPP_DONE
#error "Cyclic include dependency"
#endif
