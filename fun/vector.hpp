#ifndef FUN_VECTOR_HPP
#define FUN_VECTOR_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>
#include <fun/idtype.hpp>

#include <algorithm>
#include <cassert>
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
}

// traits

template <typename> struct fun_traits;
template <typename T, typename Allocator>
struct fun_traits<std::vector<T, Allocator>> {
  template <typename U>
  using constructor =
      std::vector<U, typename Allocator::template rebind<U>::other>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 1;
  typedef adt::index_t<rank> index_type;
  typedef adt::idtype<adt::dummy> boundary_dummy;
};

// iotaMap

template <
    typename C, typename F, typename... Args,
    std::enable_if_t<detail::is_vector<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<const F &, std::ptrdiff_t, const Args &...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(const F &f, const adt::irange_t &inds, const Args &... args) {
  std::ptrdiff_t s = inds.size();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, inds[i], args...);
  return rs;
}

namespace detail {
struct vector_iotaMapMulti {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, F &&f, Args &&... args) const {
    return cxx::invoke(std::forward<F>(f), adt::set<adt::index_t<1>>(i),
                       std::forward<Args>(args)...);
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_vector<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, adt::index_t<D> s, Args &&... args) {
  static_assert(D == 1, "");
  return iotaMap<C>(detail::vector_iotaMapMulti(), s[0], std::forward<F>(f),
                    std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, typename Allocator, typename... Args,
          typename CT = std::vector<T, Allocator>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<
              std::vector<T, Allocator>>::template constructor<R>>
CR fmap(F &&f, const std::vector<T, Allocator> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], args...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename... Args,
          typename CT = std::vector<T, Allocator>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<
              std::vector<T, Allocator>>::template constructor<R>>
CR fmap(F &&f, std::vector<T, Allocator> &&xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, std::move(xs[i]), args...);
  return rs;
}

template <typename F, typename T, typename Allocator, typename T2,
          typename Allocator2, typename... Args,
          typename CT = std::vector<T, Allocator>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<
              std::vector<T, Allocator>>::template constructor<R>>
CR fmap2(F &&f, const std::vector<T, Allocator> &xs,
         const std::vector<T2, Allocator2> &ys, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  assert(ys.size() == s);
  CR rs(s);
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
          typename CT = std::vector<T, Allocator>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const std::vector<T, Allocator> &xs,
               std::size_t bmask, BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  std::ptrdiff_t s = xs.size();
  CR rs(s);
  if (__builtin_expect(s == 1, false)) {
    rs[0] = cxx::invoke(std::forward<F>(f), xs[0], bmask, std::forward<BM>(bm),
                        std::forward<BP>(bp), std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], bmask & 0b01, std::forward<BM>(bm),
                        cxx::invoke(g, xs[1], 0), args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], 0b00, cxx::invoke(g, xs[i - 1], 1),
                          cxx::invoke(g, xs[i + 1], 0), args...);
    rs[s - 1] =
        cxx::invoke(f, xs[s - 1], bmask & 0b10, cxx::invoke(g, xs[s - 2], 1),
                    std::forward<BP>(bp), args...);
  }
  return rs;
}

template <std::size_t D, typename F, typename G, typename T, typename Allocator,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = std::vector<T, Allocator>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const std::vector<T, Allocator> &xs,
                    const BCB &bm, const BCB &bp, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
  if (__builtin_expect(s == 1, false)) {
    rs[0] = cxx::invoke(std::forward<F>(f), xs[0], 0b11, mextract(bm),
                        mextract(bp), std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], 0b01, mextract(bm), cxx::invoke(g, xs[1], 0),
                        args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], 0b00, cxx::invoke(g, xs[i - 1], 1),
                          cxx::invoke(g, xs[i + 1], 0), args...);
    rs[s - 1] = cxx::invoke(f, xs[s - 1], 0b10, cxx::invoke(g, xs[s - 2], 1),
                            mextract(bp), args...);
  }
  return rs;
}

// head, last

template <typename T, typename Allocator>
const T &head(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs.front();
}

template <typename T, typename Allocator>
const T &last(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs.back();
}

template <typename T, typename Allocator>
T &&head(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs.front());
}

template <typename T, typename Allocator>
T &&last(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs.back());
}

// boundary

template <typename T, typename Allocator,
          typename CT = std::vector<T, Allocator>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const std::vector<T, Allocator> &xs, std::ptrdiff_t i) {
  assert(i >= 0 && i < 2);
  return munit<BC>(i == 0 ? head(xs) : last(xs));
}

// boundaryMap

template <typename F, typename T, typename Allocator, typename... Args,
          typename CT = std::vector<T, Allocator>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const std::vector<T, Allocator> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// indexing

template <typename T, typename Allocator>
const T &getIndex(const std::vector<T, Allocator> &xs, std::ptrdiff_t i) {
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

// template <typename F, typename T, typename Allocator, typename... Args>
// auto fmapIndexed(F &&f, const std::vector<T, Allocator> &xs, Args &&... args)
// {
//   typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...> R;
//   typedef
//       typename fun_traits<std::vector<T, Allocator>>::template constructor<R>
//           CR;
//   std::ptrdiff_t s = xs.size();
//   CR rs(s);
// #pragma omp simd
//   for (std::ptrdiff_t i = 0; i < s; ++i)
//     rs[i] = cxx::invoke(f, xs[i], i, args...);
//   return rs;
// }

// foldMap

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
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
R foldMap(F &&f, Op &&op, Z &&z, std::vector<T, Allocator> &&xs,
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
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
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

template <typename C, typename T,
          std::enable_if_t<detail::is_vector<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return CR{std::forward<T>(x)};
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
CR mbind(F &&f, const std::vector<T, Allocator> &xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, typename Allocator, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, std::vector<T, Allocator> &&xs, Args &&... args) {
  static_assert(detail::is_vector<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T, typename Allocator>
const T &mextract(const std::vector<T, Allocator> &xs) {
  assert(!xs.empty());
  return xs[0];
}

template <typename T, typename Allocator>
T &&mextract(std::vector<T, Allocator> &&xs) {
  assert(!xs.empty());
  return std::move(xs[0]);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename Allocator,
          typename... Args, typename CT = std::vector<T, Allocator>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const std::vector<T, Allocator> &xs,
            Args &&... args) {
  return munit<typename fun_traits<CT>::dummy>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_vector<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return CR();
}

// mplus

template <typename T, typename Allocator, typename... Ts,
          typename... Allocators, typename CT = std::vector<T, Allocator>>
CT mplus(const std::vector<T, Allocator> &xs,
         const std::vector<Ts, Allocators> &... yss) {
  CT rs(xs);
  for (auto pys : std::initializer_list<const CT *>{&yss...})
    rs.insert(rs.end(), pys->begin(), pys->end());
  return rs;
}

template <typename T, typename Allocator, typename... Ts,
          typename... Allocators, typename CT = std::vector<T, Allocator>>
CT mplus(std::vector<T, Allocator> &&xs,
         std::vector<Ts, Allocators> &&... yss) {
  CT rs(std::move(xs));
  for (auto pys : std::initializer_list<CT *>{&yss...})
    std::move(pys->begin(), pys->end(), std::back_inserter(rs));
  return rs;
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_vector<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR msome(T &&x, Ts &&... ys) {
  return CR{std::forward<T>(x), std::forward<Ts>(ys)...};
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
