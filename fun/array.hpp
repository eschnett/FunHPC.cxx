#ifndef FUN_ARRAY_HPP
#define FUN_ARRAY_HPP

#include <adt/array.hpp>
#include <cxx/invoke.hpp>
#include <fun/idtype.hpp>
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
  typedef adt::index_t<1> index_type;
  template <typename U> using boundary_constructor = adt::idtype<U>;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          std::enable_if_t<detail::is_array<C<int>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  assert(s == fun_traits<C<R>>::size);
  C<R> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, i, args...);
  return rs;
}

namespace detail {
struct array_iotaMap_wrapper {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, F &&f, Args &&... args) const {
    return cxx::invoke(std::forward<F>(f), adt::set<adt::index_t<1>>(i),
                       std::forward<Args>(args)...);
  }
};
}

template <template <typename> class C, typename F, typename... Args,
          std::enable_if_t<detail::is_array<C<int>>::value> * = nullptr>
auto iotaMap(F &&f, adt::index_t<1> s, Args &&... args) {
  return iotaMap<C>(detail::array_iotaMap_wrapper(), s[0], std::forward<F>(f),
                    std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, std::size_t N, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const std::array<T, N> &xs, Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::array<T, N> &&xs, Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, std::move(xs[i]), args...);
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
    rs[i] = cxx::invoke(f, xs[i], ys[i], args...);
  return rs;
}
// fmapStencil

template <
    typename F, typename G, typename T, std::size_t N, typename... Args,
    typename B = cxx::invoke_of_t<G, T, bool, std::ptrdiff_t>,
    typename R = cxx::invoke_of_t<F, T, std::tuple<B>, std::tuple<B>, Args...>>
auto fmapStencil(F &&f, G &&g, const std::array<T, N> &xs,
                 const std::tuple<B> &bl, const std::tuple<B> &bu,
                 Args &&... args) {
  std::ptrdiff_t s = N;
  std::array<R, N> rs;
  if (__builtin_expect(s == 1, false)) {
    rs[0] = cxx::invoke(std::forward<F>(f), xs[0], bl, bu,
                        std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], bl,
                        std::forward_as_tuple(cxx::invoke(g, xs[1], false, 0)),
                        args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(
          f, xs[i], std::forward_as_tuple(cxx::invoke(g, xs[i - 1], true, 0)),
          std::forward_as_tuple(cxx::invoke(g, xs[i + 1], false, 0)), args...);
    rs[s - 1] = cxx::invoke(
        f, xs[s - 1], std::forward_as_tuple(cxx::invoke(g, xs[s - 2], true, 0)),
        bu, args...);
  }
  return rs;
}

// head, last

template <typename T, std::size_t N>
decltype(auto) head(const std::array<T, N> &xs) {
  static_assert(!xs.empty(), "");
  return xs.front();
}

template <typename T, std::size_t N>
decltype(auto) last(const std::array<T, N> &xs) {
  static_assert(!xs.empty(), "");
  return xs.back();
}

template <typename T, std::size_t N>
decltype(auto) head(std::array<T, N> &&xs) {
  static_assert(!xs.empty(), "");
  return std::move(xs.front());
}

template <typename T, std::size_t N>
decltype(auto) last(std::array<T, N> &&xs) {
  static_assert(!xs.empty(), "");
  return std::move(xs.back());
}

// boundary

template <typename T, std::size_t N,
          template <typename> class BC =
              fun_traits<std::array<T, N>>::template boundary_constructor>
auto boundary(const std::array<T, N> &xs, std::ptrdiff_t i) {
  assert(i >= 0 && i < 2);
  return munit<BC>(i == 0 ? head(xs) : last(xs));
}

// boundaryMap

template <typename F, typename T, std::size_t N, typename... Args>
auto boundaryMap(F &&f, const std::array<T, N> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), std::forward<Args>(args)...);
}

// indexing

template <typename T, std::size_t N>
decltype(auto) getIndex(const std::array<T, N> &xs, std::ptrdiff_t i) {
  static_assert(!xs.empty(), "");
  return xs[i];
}

template <typename> class accumulator;
template <typename T, std::size_t N> class accumulator<std::array<T, N>> {
  std::array<T, N> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  decltype(auto) finalize() { return std::move(data); }
};

template <typename F, typename T, std::size_t N, typename... Args,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>>
auto fmapIndexed(F &&f, const std::array<T, N> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  typename fun_traits<std::array<T, N>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], i, args...);
  return rs;
}

// foldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, std::array<T, N> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r),
                    cxx::invoke(f, std::move(xs[i]), args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs,
           const std::array<T2, N> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = N;
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], args...));
  return r;
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_array<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  C<R> rs;
  for (auto &r : rs)
    r = x;
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
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs,
              Args &&... args) {
  return munit<fun_traits<std::array<T, N>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
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

template <typename T, std::size_t N>
constexpr bool mempty(const std::array<T, N> &xs) {
  return xs.empty();
}

// msize

template <typename T, std::size_t N>
constexpr std::size_t msize(const std::array<T, N> &xs) {
  return xs.size();
}
}

#define FUN_ARRAY_HPP_DONE
#endif // #ifdef FUN_ARRAY_HPP
#ifndef FUN_ARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
