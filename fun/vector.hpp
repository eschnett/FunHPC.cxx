#ifndef FUN_VECTOR_HPP
#define FUN_VECTOR_HPP

#include <cxx/invoke.hpp>
#include <fun/topology.hpp>

#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <vector>
#include <tuple>
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

// traits

template <typename> struct fun_traits;
template <typename T, typename Allocator>
struct fun_traits<std::vector<T, Allocator>> {
  template <typename U>
  using constructor =
      std::vector<U, typename Allocator::template rebind<U>::other>;
  typedef T value_type;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  C<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), i, std::forward<Args>(args)...);
  return rs;
}

template <template <typename, typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return iotaMap<detail::vector1>(std::forward<F>(f), s,
                                  std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, typename Allocator, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const std::vector<T, Allocator> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  typename fun_traits<std::vector<T, Allocator>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::vector<T, Allocator> &&xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  typename fun_traits<std::vector<T, Allocator>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), std::move(xs[i]),
                        std::forward<Args>(args)...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const std::vector<T, Allocator> &xs,
           const std::vector<T2, Allocator2> &ys, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  assert(ys.size() == s);
  typename fun_traits<std::vector<T, Allocator>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(std::forward<F>(f), xs[i], ys[i],
                        std::forward<Args>(args)...);
  return rs;
}

// fmapTopo

template <typename F, typename G, typename T, typename Allocator,
          typename... Args, typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, connectivity<B>, Args...>>
auto fmapTopo(F &&f, G &&g, const std::vector<T, Allocator> &xs,
              const connectivity<B> &bs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  std::vector<R, Allocator> rs(s);
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

template <typename T, typename Allocator>
decltype(auto) head(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs.front();
}

template <typename T, typename Allocator>
decltype(auto) last(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename T, typename Allocator>
decltype(auto) head(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs.front());
}

template <typename T, typename Allocator>
decltype(auto) last(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs.back());
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto foldMap(F &&f, Op &&op, const Z &z, const std::vector<T, Allocator> &xs,
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

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto foldMap(F &&f, Op &&op, const Z &z, std::vector<T, Allocator> &&xs,
             Args &&... args) {
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

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename T2, typename Allocator2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
auto foldMap2(F &&f, Op &&op, const Z &z, const std::vector<T, Allocator> &xs,
              const std::vector<T2, Allocator2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  assert(ys.size() == s);
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
  return C<R>{std::forward<T>(x)};
}

template <template <typename, typename> class C, typename T,
          typename R = std::decay_t<T>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto munit(T &&x) {
  return munit<detail::vector1>(std::forward<T>(x));
}

// mjoin

template <typename T, typename Allocator, typename Allocator2>
auto mjoin(const std::vector<std::vector<T, Allocator>, Allocator2> &xss) {
  std::vector<T, Allocator> rs;
  for (const auto &xs : xss)
    rs.insert(rs.end(), xs.begin(), xs.end());
  return rs;
}

template <typename T, typename Allocator, typename Allocator2>
auto mjoin(std::vector<std::vector<T, Allocator>, Allocator2> &&xss) {
  std::vector<T, Allocator> rs;
  for (auto &xs : xss)
    std::move(xs.begin(), xs.end(), std::back_inserter(rs));
  return rs;
}

// mbind

template <typename F, typename T, typename Allocator, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const std::vector<T, Allocator> &xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename Allocator, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, std::vector<T, Allocator> &&xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T, typename Allocator>
decltype(auto) mextract(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs[0];
}

template <typename T, typename Allocator>
decltype(auto) mextract(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs[0]);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
auto mfoldMap(F &&f, Op &&op, const Z &z, const std::vector<T, Allocator> &xs,
              Args &&... args) {
  return munit<fun_traits<std::vector<T, Allocator>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), z, xs,
              std::forward<Args>(args)...));
}

// mzero

template <template <typename> class C, typename R,
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto mzero() {
  return C<R>();
}

template <template <typename, typename> class C, typename R,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto mzero() {
  return mzero<detail::vector1, R>();
}

// mplus

template <typename T, typename Allocator, typename... Ts>
auto mplus(const std::vector<T, Allocator> &xs,
           const std::vector<Ts, Allocator> &... yss) {
  std::vector<T, Allocator> rs(xs);
  for (auto pys :
       std::initializer_list<const std::vector<T, Allocator> *>{&yss...})
    rs.insert(rs.end(), pys->begin(), pys->end());
  return rs;
}

template <typename T, typename Allocator, typename... Ts>
auto mplus(std::vector<T, Allocator> &&xs,
           std::vector<Ts, Allocator> &&... yss) {
  std::vector<T, Allocator> rs(std::move(xs));
  for (auto pys : std::initializer_list<std::vector<T, Allocator> *>{&yss...})
    std::move(pys->begin(), pys->end(), std::back_inserter(rs));
  return rs;
}

// msome

template <template <typename> class C, typename T, typename... Ts,
          typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
  return C<R>{std::forward<T>(x), std::forward<Ts>(ys)...};
}

template <template <typename, typename> class C, typename T, typename... Ts,
          typename R = std::decay_t<T>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
  return msome<detail::vector1>(std::forward<T>(x), std::forward<Ts>(ys)...);
}

// mempty

template <typename T, typename Allocator>
bool mempty(const std::vector<T, Allocator> &xs) {
  return xs.empty();
}
}

#define FUN_VECTOR_HPP_DONE
#endif // #ifdef FUN_VECTOR_HPP
#ifndef FUN_VECTOR_HPP_DONE
#error "Cyclic include dependency"
#endif
