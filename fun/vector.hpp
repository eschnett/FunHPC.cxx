#ifndef FUN_VECTOR_HPP
#define FUN_VECTOR_HPP

#include <adt/array.hpp>
#include <cxx/invoke.hpp>
#include <fun/idtype.hpp>
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

template <typename T> using std_vector = std::vector<T>;
}

// traits

template <typename> struct fun_traits;
template <typename T, typename Allocator>
struct fun_traits<std::vector<T, Allocator>> {
  template <typename U>
  using constructor =
      std::vector<U, typename Allocator::template rebind<U>::other>;
  typedef T value_type;
  typedef adt::index_t<1> index_type;
  template <typename U> using boundary_constructor = adt::idtype<U>;
};

// iotaMap

template <template <typename> class C, typename F, typename... Args,
          std::enable_if_t<detail::is_vector<C<int>>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef cxx::invoke_of_t<F, std::ptrdiff_t, Args...> R;
  C<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, i, args...);
  return rs;
}

template <
    template <typename, typename> class C, typename F, typename... Args,
    std::enable_if_t<detail::is_vector<C<int, std::allocator<int>>>::value> * =
        nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  return iotaMap<detail::std_vector>(std::forward<F>(f), s,
                                     std::forward<Args>(args)...);
}

namespace detail {
struct vector_iotaMap_wrapper {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, F &&f, Args &&... args) const {
    return cxx::invoke(std::forward<F>(f), adt::set<adt::index_t<1>>(i),
                       std::forward<Args>(args)...);
  }
};
}

template <template <typename> class C, typename F, typename... Args,
          std::enable_if_t<detail::is_vector<C<int>>::value> * = nullptr>
auto iotaMap(F &&f, adt::index_t<1> s, Args &&... args) {
  return iotaMap<C>(detail::vector_iotaMap_wrapper(), s[0], std::forward<F>(f),
                    std::forward<Args>(args)...);
}

template <
    template <typename, typename> class C, typename F, typename... Args,
    std::enable_if_t<detail::is_vector<C<int, std::allocator<int>>>::value> * =
        nullptr>
auto iotaMap(F &&f, adt::index_t<1> s, Args &&... args) {
  return iotaMap<detail::std_vector>(std::forward<F>(f), s[0],
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
    rs[i] = cxx::invoke(f, xs[i], args...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, std::vector<T, Allocator> &&xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  typename fun_traits<std::vector<T, Allocator>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, std::move(xs[i]), args...);
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
    rs[i] = cxx::invoke(f, xs[i], ys[i], args...);
  return rs;
}

// fmapStencil

// probably want a datatype for the boundary. should support
// conversion to const references (as for argument passing), and
// should support decaying (as for storing, serializing). could be
// built on tuples, or on arrays (with using pointers instead of
// references).

template <typename F, typename G, typename T, typename Allocator, typename BM,
          typename BP, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>>
auto fmapStencil(F &&f, G &&g, const std::vector<T, Allocator> &xs, BM &&bm,
                 BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  std::ptrdiff_t s = xs.size();
  std::vector<R, Allocator> rs(s);
  if (__builtin_expect(s == 1, false)) {
    rs[0] = cxx::invoke(std::forward<F>(f), xs[0], 0b11, std::forward<BM>(bm),
                        std::forward<BP>(bp), std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], 0b01, bm, cxx::invoke(g, xs[1], 0), args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], 0b00, cxx::invoke(g, xs[i - 1], 1),
                          cxx::invoke(g, xs[i + 1], 0), args...);
    rs[s - 1] = cxx::invoke(f, xs[s - 1], 0b10, cxx::invoke(g, xs[s - 2], 1),
                            bp, args...);
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

// boundary

template <typename T, typename Allocator,
          template <typename> class BC = fun_traits<
              std::vector<T, Allocator>>::template boundary_constructor>
auto boundary(const std::vector<T, Allocator> &xs, std::ptrdiff_t i) {
  assert(i >= 0 && i < 2);
  return munit<BC>(i == 0 ? head(xs) : last(xs));
}

// boundaryMap

template <typename F, typename T, typename Allocator, typename... Args>
auto boundaryMap(F &&f, const std::vector<T, Allocator> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), std::forward<Args>(args)...);
}

// indexing

template <typename T, typename Allocator>
decltype(auto) getIndex(const std::vector<T, Allocator> &xs, std::ptrdiff_t i) {
  return xs[i];
}

template <typename> class accumulator;
template <typename T, typename Allocator>
class accumulator<std::vector<T, Allocator>> {
  std::vector<T, Allocator> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  decltype(auto) finalize() { return std::move(data); }
  ~accumulator() { assert(data.empty()); }
};

template <typename F, typename T, typename Allocator, typename... Args,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args &...>>
auto fmapIndexed(F &&f, const std::vector<T, Allocator> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  typename fun_traits<std::vector<T, Allocator>>::template constructor<R> rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], i, args...);
  return rs;
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
auto foldMap(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
             Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  R r(z);
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
auto foldMap(F &&f, Op &&op, Z &&z, std::vector<T, Allocator> &&xs,
             Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  R r(z);
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r),
                    cxx::invoke(f, std::move(xs[i]), args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename T2, typename Allocator2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
auto foldMap2(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
              const std::vector<T2, Allocator2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  assert(ys.size() == s);
  R r(z);
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
          std::enable_if_t<detail::is_vector<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return C<R>{std::forward<T>(x)};
}

template <template <typename, typename> class C, typename T,
          typename R = std::decay_t<T>,
          std::enable_if_t<
              detail::is_vector<C<R, std::allocator<R>>>::value> * = nullptr>
auto munit(T &&x) {
  return munit<detail::std_vector>(std::forward<T>(x));
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
auto mfoldMap(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
              Args &&... args) {
  return munit<fun_traits<std::vector<T, Allocator>>::template constructor>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
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
  return mzero<detail::std_vector, R>();
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
  return msome<detail::std_vector>(std::forward<T>(x), std::forward<Ts>(ys)...);
}

// mempty

template <typename T, typename Allocator>
bool mempty(const std::vector<T, Allocator> &xs) {
  return xs.empty();
}

// msize

template <typename T, typename Allocator>
std::size_t msize(const std::vector<T, Allocator> &xs) {
  return xs.size();
}
}

#define FUN_VECTOR_HPP_DONE
#endif // #ifdef FUN_VECTOR_HPP
#ifndef FUN_VECTOR_HPP_DONE
#error "Cyclic include dependency"
#endif
