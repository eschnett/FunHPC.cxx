#ifndef FUN_NESTED_HPP
#define FUN_NESTED_HPP

#include <adt/nested.hpp>
#include <cxx/invoke.hpp>
#include <fun/topology.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <type_traits>
#include <tuple>
#include <utility>

namespace fun {

// is_nested

namespace detail {
template <typename> struct is_nested : std::false_type {};
template <template <typename> class P, template <typename> class A, typename T>
struct is_nested<adt::nested<P, A, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <template <typename> class P, template <typename> class A, typename T>
struct fun_traits<adt::nested<P, A, T>> {
  template <typename U> using constructor = adt::nested<P, A, U>;
  typedef T value_type;
};

// iotaMap

namespace detail {
template <template <typename> class A> struct nested_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, std::ptrdiff_t s, F &&f,
                  Args &&... args) const {
    return iotaMap<A>(f, s, args...);
  }
};
}

template <template <typename> class C, typename F, typename... Args,
          typename R = std::decay_t<cxx::invoke_of_t<
              std::decay_t<F>, std::ptrdiff_t, std::decay_t<Args>...>>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return {iotaMap<P>(detail::nested_iotaMap<A>(), std::ptrdiff_t(1), s,
                     std::forward<F>(f), std::forward<Args>(args)...)};
}

// fmap

namespace detail {
template <template <typename> class A, typename T>
struct nested_fmap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(const A<T> &xs, F &&f, Args &&... args) const {
    return fmap(f, xs, args...);
  }
};
}

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
adt::nested<P, A, R> fmap(F &&f, const adt::nested<P, A, T> &xss,
                          Args &&... args) {
  return {fmap(detail::nested_fmap<A, T>(), xss.data, std::forward<F>(f),
               std::forward<Args>(args)...)};
}

namespace detail {
template <template <typename> class A, typename T, typename T2>
struct nested_fmap2 : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(const A<T> &xs, const A<T2> &ys, F &&f,
                  Args &&... args) const {
    return fmap2(f, xs, ys, args...);
  }
};
}

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
adt::nested<P, A, R> fmap2(F &&f, const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, T2> &yss, Args &&... args) {
  return {fmap2(detail::nested_fmap2<A, T, T2>(), xss.data, yss.data,
                std::forward<F>(f), std::forward<Args>(args)...)};
}

// fmapTopo

namespace detail {
template <template <typename> class A, typename T, typename B>
struct nested_fmapTopo : std::tuple<> {
  template <typename F, typename G, typename... Args>
  auto operator()(const A<T> &xs, F &&f, G &&g, const connectivity<B> &bs,
                  Args &&... args) const {
    return fmapTopo(f, g, xs, bs, args...);
  }
};
}

template <typename F, typename G, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, connectivity<B>, Args...>>
adt::nested<P, A, R> fmapTopo(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                              const connectivity<B> &bs, Args &&... args) {
  return {fmap(detail::nested_fmapTopo<A, T, B>(), xss.data, std::forward<F>(f),
               std::forward<G>(g), bs, std::forward<Args>(args)...)};
}

// head, last

namespace detail {
template <template <typename> class A, typename T>
struct nested_head : std::tuple<> {
  auto operator()(const A<T> &xs) const { return head(xs); }
};

template <template <typename> class A, typename T>
struct nested_last : std::tuple<> {
  auto operator()(const A<T> &xs) const { return last(xs); }
};
}

// Note: The call to fmap is necessary in case we use a proxy; calling
// head or mextract on the proxy would copy the whole data structure
// to the local process, which would be prohibitively expensive.

// Note: The call to fmap creates a temporary, and mextract returns a
// reference to the content of this temporary. It appears that Clang
// extends the lifetime of this temporary (maybe accidentally?), while
// GCC does not. Hence we need to return a value and not a reference.
template <template <typename> class P, template <typename> class A, typename T>
auto head(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_head<A, T>(), xss.data));
}

template <template <typename> class P, template <typename> class A, typename T>
auto last(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_last<A, T>(), xss.data));
}

// foldMap

namespace detail {
template <template <typename> class A, typename T>
struct nested_foldMap : std::tuple<> {
  template <typename F, typename Op, typename Z, typename... Args>
  auto operator()(const A<T> &xs, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                   xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap(detail::nested_foldMap<A, T>(), op, z, xss.data,
                 std::forward<F>(f), op, z, std::forward<Args>(args)...);
}

namespace detail {
template <template <typename> class A, typename T, typename T2>
struct nested_foldMap2 : std::tuple<> {
  template <typename F, typename Op, typename Z, typename... Args>
  auto operator()(const A<T> &xs, const A<T2> &ys, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return foldMap2(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), xs, ys, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
           const adt::nested<P, A, T2> &yss, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap2(detail::nested_foldMap2<A, T, T2>(), op, z, xss.data,
                  yss.data, std::forward<F>(f), op, z,
                  std::forward<Args>(args)...);
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> munit(T &&x) {
  return {munit<P>(munit<A>(std::forward<T>(x)))};
}

// mjoin

namespace detail {
template <template <typename> class P, template <typename> class A, typename T>
struct nested_mextract : std::tuple<> {
  auto operator()(const adt::nested<P, A, T> &xss) const {
    return mextract(xss.data);
  }
};

template <template <typename> class P, template <typename> class A, typename T>
struct nested_mjoin : std::tuple<> {
  auto operator()(const A<adt::nested<P, A, T>> &xsss) const {
    return mjoin(fmap(nested_mextract<P, A, T>(), xsss));
  }
};
}

template <template <typename> class P, template <typename> class A, typename T>
adt::nested<P, A, T>
mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss) {
  return {fmap(detail::nested_mjoin<P, A, T>(), xssss.data)};
}

// mbind

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>,
          typename R = typename CR::value_type>
adt::nested<P, A, R> mbind(F &&f, const adt::nested<P, A, T> &xss,
                           Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xss, std::forward<Args>(args)...));
}

// mextract

template <template <typename> class P, template <typename> class A, typename T>
auto mextract(const adt::nested<P, A, T> &xss) {
  return mextract(mextract(xss.data));
}

// mfoldMap

namespace detail {
template <template <typename> class A, typename T>
struct nested_mfoldMap : std::tuple<> {
  template <typename F, typename Op, typename Z, typename... Args>
  auto operator()(const A<T> &xs, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return mfoldMap(f, op, z, xs, args...);
  }
};
}

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
adt::nested<P, A, R> mfoldMap(F &&f, Op &&op, Z &&z,
                              const adt::nested<P, A, T> &xss,
                              Args &&... args) {
  return {fmap(detail::nested_mfoldMap<A, T>(), xss.data, std::forward<F>(f),
               std::forward<Op>(op), std::forward<Z>(z),
               std::forward<Args>(args)...)};
}

// mzero

template <template <typename> class C, typename R,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor>
C<R> mzero() {
  return {mzero<P, A<R>>()};
  // return {munit<P>(mzero<A, R>())};
}

// mplus

template <template <typename> class P, template <typename> class A, typename T,
          typename... Ts>
adt::nested<P, A, T> mplus(const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, Ts> &... yss) {
  return {munit<P, A<T>>(
      mplus(mempty(xss.data) ? mzero<A, T>() : mextract(xss.data),
            mempty(yss.data) ? mzero<A, T>() : mextract(yss.data)...))};
}

// msome

template <template <typename> class C, typename T, typename... Ts,
          typename R = std::decay_t<T>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> msome(T &&x, Ts &&... ys) {
  return {munit<P>(msome<A>(std::forward<T>(x), std::forward<Ts>(ys)...))};
}

// mempty

template <template <typename> class P, template <typename> class A, typename T>
bool mempty(const adt::nested<P, A, T> &xss) {
  return mempty(xss.data);
}
}

#define FUN_NESTED_HPP_DONE
#endif // #ifdef FUN_NESTED_HPP
#ifndef FUN_NESTED_HPP_DONE
#error "Cyclic include dependency"
#endif
