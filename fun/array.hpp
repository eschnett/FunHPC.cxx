#ifndef FUN_ARRAY_HPP
#define FUN_ARRAY_HPP

#include <adt/array.hpp>
#include <cxx/invoke.hpp>
#include <fun/topology.hpp>

#include <array>
#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fun {

// is_array

namespace detail {
template <typename> struct is_array : std::false_type {};
template <typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, std::size_t N> struct fun_traits<std::array<T, N>> {
  template <typename U> using constructor = std::array<U, N>;
  typedef T value_type;
  static constexpr std::size_t size = N;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_array<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  assert(s == fun_traits<C<R>>::size);
  C<R> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), i, std::forward<Args>(args)...);
  return rs;
}

// fmap

template <typename F, typename T, std::size_t N, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const std::array<T, N> &xs, Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::array<T, N> &&xs, Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), std::move(xs[i]),
                        std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const std::array<T, N> &xs, const std::array<T2, N> &ys,
           Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], ys[i],
                        std::forward<Args>(args)...);
  return rs;
}

// fmapTopo

template <typename F, typename G, typename T, std::size_t N, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, connectivity<B>, Args...>>
auto fmapTopo(F &&f, G &&g, const std::array<T, N> &xs,
              const connectivity<B> &bs, Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
  if (s == 1) {
    rs[0] =
        cxx::invoke(std::forward<F>(f), xs[0], bs, std::forward<Args>(args)...);
  } else if (s > 1) {
    rs[0] = cxx::invoke(
        std::forward<F>(f), xs[0],
        connectivity<B>(get<0>(bs), cxx::invoke(std::forward<G>(g), xs[1], 0)),
        std::forward<Args>(args)...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(
          f, xs[i],
          connectivity<B>(cxx::invoke(std::forward<G>(g), xs[i - 1], 1),
                          cxx::invoke(std::forward<G>(g), xs[i + 1], 0)),
          std::forward<Args>(args)...);
    rs[s - 1] = cxx::invoke(
        std::forward<F>(f), xs[s - 1],
        connectivity<B>(cxx::invoke(std::forward<G>(g), xs[s - 2], 1),
                        get<1>(bs)),
        std::forward<Args>(args)...);
  }
  return rs;
}

// head, last

template <typename T, std::size_t N>
decltype(auto) head(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs.front();
}

template <typename T, std::size_t N>
decltype(auto) last(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename T, std::size_t N>
decltype(auto) head(std::array<T, N> &&xs) {
  assert(!xs.empty());
  return std::move(xs.front());
}

template <typename T, std::size_t N>
decltype(auto) last(std::array<T, N> &&xs) {
  assert(!xs.empty());
  return std::move(xs.back());
}

// foldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, const std::array<T, N> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(z);
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(std::forward < Op > (op), std::move(omp_out),        \
                          omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(
        std::forward<Op>(op), std::move(r),
        cxx::invoke(std::forward<F>(f), xs[i], std::forward<Args>(args)...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, std::array<T, N> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(z);
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(std::forward < Op > (op), std::move(omp_out),        \
                          omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(std::forward<Op>(op), std::move(r),
                    cxx::invoke(std::forward<F>(f), std::move(xs[i]),
                                std::forward<Args>(args)...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, const Z &z, const std::array<T, N> &xs,
           const std::array<T2, N> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(z);
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(std::forward < Op > (op), std::move(omp_out),        \
                          omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(std::forward<Op>(op), std::move(r),
                    cxx::invoke(std::forward<F>(f), xs[i], ys[i],
                                std::forward<Args>(args)...));
  return r;
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_array<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  C<R> rs;
  for (auto &r : rs)
    r = std::forward<T>(x);
  return rs;
}

// mjoin

template <typename T, std::size_t N, std::size_t N2>
auto mjoin(const std::array<std::array<T, N>, N2> &xss) {
  std::array<T, N * N2> rs;
  for (std::size_t i = 0; i < N2; ++i)
    std::copy(xss[i].begin(), xss[i].end(), rs.begin() + N * i);
  return rs;
}

template <typename T, std::size_t N, std::size_t N2>
auto mjoin(std::array<std::array<T, N>, N2> &&xss) {
  std::array<T, N * N2> rs;
  for (std::size_t i = 0; i < N2; ++i)
    std::move(xss[i].begin(), xss[i].end(), rs.begin() + N * i);
  return rs;
}

// mbind

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const std::array<T, N> &xs, Args &&... args) {
  static_assert(detail::is_array<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, std::array<T, N> &&xs, Args &&... args) {
  static_assert(detail::is_array<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T, std::size_t N>
decltype(auto) mextract(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs[0];
}

template <typename T, std::size_t N>
decltype(auto) mextract(std::array<T, N> &&xs) {
  assert(!xs.empty());
  return std::move(xs[0]);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto mfoldMap(F &&f, Op &&op, const Z &z, const std::array<T, N> &xs,
              Args &&... args) {
  return munit<fun_traits<std::array<T, N>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), z, xs,
              std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_array<C<R>>::value> * = nullptr>
auto mzero() {
  static_assert(fun_traits<C<R>>::size == 0, "");
  return C<R>();
}

// mempty

template <typename T, std::size_t N> bool mempty(const std::array<T, N> &xs) {
  return xs.empty();
}
}

#define FUN_ARRAY_HPP_DONE
#endif // #ifdef FUN_ARRAY_HPP
#ifndef FUN_ARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
