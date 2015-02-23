#ifndef FUN_VECTOR_HPP
#define FUN_VECTOR_HPP

#include <cxx/invoke.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <vector>
#include <type_traits>
#include <utility>

namespace fun {

// is_vector

namespace detail {
template <typename> struct is_vector : std::false_type {};
template <typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type {};

template <typename T> using vector1 = std::vector<T>;
}

// iota

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto iota(F &&f, std::ptrdiff_t s, Args &&... args) {
  std::vector<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), i, std::forward<Args>(args)...);
  return rs;
}

// TODO: Use rebind to obtain the new allocator
template <template <typename, typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto iota(F &&f, std::ptrdiff_t s, Args &&... args) {
  return iota<detail::vector1>(std::forward<F>(f), s,
                               std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const std::vector<T> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  std::vector<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::vector<T> &&xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  std::vector<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), std::move(xs[i]),
                        std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const std::vector<T> &xs, const std::vector<T2> &ys,
           Args &&... args) {
  std::ptrdiff_t s = xs.size();
  assert(ys.size() == s);
  std::vector<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], ys[i],
                        std::forward<Args>(args)...);
  return rs;
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, const std::vector<T> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
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

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, const Z &z, std::vector<T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
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

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, const Z &z, const std::vector<T> &xs,
           const std::vector<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
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
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return std::vector<R>{std::forward<T>(x)};
}

template <template <typename, typename> class C, typename T,
          typename R = std::decay_t<T>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto munit(T &&x) {
  return munit<detail::vector1>(std::forward<T>(x));
}

// mjoin

template <typename T> auto mjoin(const std::vector<std::vector<T>> &xss) {
  std::vector<T> rs;
  for (const auto &xs : xss)
    rs.insert(rs.end(), xs.begin(), xs.end());
  return rs;
}

template <typename T> auto mjoin(std::vector<std::vector<T>> &&xss) {
  std::vector<T> rs;
  for (auto &xs : xss)
    std::move(xs.begin(), xs.end(), std::back_inserter(rs));
  return rs;
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const std::vector<T> &xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, std::vector<T> &&xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T> decltype(auto) mextract(const std::vector<T> &xs) {
  assert(!xs.empty());
  return xs[0];
}

template <typename T> decltype(auto) mextract(std::vector<T> &&xs) {
  assert(!xs.empty());
  return std::move(xs[0]);
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto mzero() {
  return std::vector<R>();
}

template <template <typename, typename> class C, typename R,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto mzero() {
  return mzero<detail::vector1, R>();
}

// mplus

template <typename T, typename... Ts>
auto mplus(const std::vector<T> &xs, const std::vector<Ts> &... yss) {
  std::vector<T> rs(xs);
  for (auto pys : std::initializer_list<const std::vector<T> *>{&yss...})
    rs.insert(rs.end(), pys->begin(), pys->end());
  return rs;
}

template <typename T, typename... Ts>
auto mplus(std::vector<T> &&xs, std::vector<Ts> &&... yss) {
  std::vector<T> rs(std::move(xs));
  for (auto pys : std::initializer_list<std::vector<T> *>{&yss...})
    std::move(pys->begin(), pys->end(), std::back_inserter(rs));
  return rs;
}
}

#define FUN_VECTOR_HPP_DONE
#endif // #ifdef FUN_VECTOR_HPP
#ifndef FUN_VECTOR_HPP_DONE
#error "Cyclic include dependency"
#endif
