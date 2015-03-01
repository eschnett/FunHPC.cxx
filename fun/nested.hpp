#ifndef FUN_NESTED_HPP
#define FUN_NESTED_HPP

#include <adt/nested.hpp>
#include <cxx/invoke.hpp>
#include <fun/topology.hpp>

#include <cereal/access.hpp>

#include <type_traits>
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

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return {iotaMap<P>([s](std::ptrdiff_t, auto &&f,
                         auto &&... args) { return iotaMap<A>(f, s, args...); },
                     std::ptrdiff_t(1), std::forward<F>(f),
                     std::forward<Args>(args)...)};
}

// fmap

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
adt::nested<P, A, R> fmap(F &&f, const adt::nested<P, A, T> &xss,
                          Args &&... args) {
  return {fmap([](const A<T> &xs, auto &&f,
                  auto &&... args) { return fmap(f, xs, args...); },
               xss.data, std::forward<F>(f), std::forward<Args>(args)...)};
}

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
adt::nested<P, A, R> fmap2(F &&f, const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, T2> &yss, Args &&... args) {
  return {fmap2([](const A<T> &xs, const A<T2> &ys, auto &&f,
                   auto &&... args) { return fmap2(f, xs, ys, args...); },
                xss.data, yss.data, std::forward<F>(f),
                std::forward<Args>(args)...)};
}

// fmapTopo

template <typename F, typename G, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, connectivity<B>, Args...>>
adt::nested<P, A, R> fmapTopo(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                              const connectivity<B> &bs, Args &&... args) {
  return {fmap([](const A<T> &xs, auto &&f, auto &&g, auto &&bs,
                  auto &&... args) { return fmapTopo(f, g, xs, bs, args...); },
               xss.data, std::forward<F>(f), std::forward<G>(g), bs,
               std::forward<Args>(args)...)};
}

// head, last

template <template <typename> class P, template <typename> class A, typename T>
decltype(auto) head(const adt::nested<P, A, T> &xss) {
  return mextract(fmap([](const A<T> &xs) { return head(xs); }, xss.data));
}

template <template <typename> class P, template <typename> class A, typename T>
decltype(auto) last(const adt::nested<P, A, T> &xss) {
  return mextract(fmap([](const A<T> &xs) { return last(xs); }, xss.data));
}

// foldMap

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, const Z &z, const adt::nested<P, A, T> &xss,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap([](const A<T> &xs, auto &&f, auto &&op, const Z &z,
                    auto &&... args) { return foldMap(f, op, z, xs, args...); },
                 op, z, xss.data, std::forward<F>(f), op, z,
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

template <template <typename> class P, template <typename> class A, typename T>
adt::nested<P, A, T>
mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss) {
  return {fmap([](const A<adt::nested<P, A, T>> &xsss) {
                 return mjoin(fmap([](const adt::nested<P, A, T> &xss) {
                                     return mextract(xss.data);
                                   },
                                   xsss));
               },
               xssss.data)};
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
decltype(auto) mextract(const adt::nested<P, A, T> &xss) {
  return mextract(mextract(xss.data));
}

// mfoldMap

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
adt::nested<P, A, R> mfoldMap(F &&f, Op &&op, const Z &z,
                              const adt::nested<P, A, T> &xss,
                              Args &&... args) {
  return {fmap([](const A<T> &xs, auto &&f, auto &&op, auto &&z,
                  auto &&... args) { return mfoldMap(f, op, z, xs, args...); },
               xss.data, std::forward<F>(f), std::forward<Op>(op), z,
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
