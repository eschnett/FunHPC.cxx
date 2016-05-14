#ifndef FUN_MAXARRAY_HPP
#define FUN_MAXARRAY_HPP

#include <adt/maxarray.hpp>

#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <cxx/cassert.hpp>
#include <fun/fun_decl.hpp>
#include <fun/idtype.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>
#include <type_traits>
#include <utility>

namespace fun {

// is_maxarray

namespace detail {
template <typename> struct is_maxarray : std::false_type {};
template <typename T, std::size_t N>
struct is_maxarray<adt::maxarray<T, N>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, std::size_t N> struct fun_traits<adt::maxarray<T, N>> {
  template <typename U> using constructor = adt::maxarray<std::decay_t<U>, N>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 1;
  typedef adt::index_t<rank> index_type;

  typedef adt::idtype<adt::dummy> boundary_dummy;

  static constexpr std::size_t min_size() { return 0; }
  static constexpr std::size_t max_size() { return N; }
};

// iotaMap

template <
    typename C, typename F, typename... Args,
    std::enable_if_t<detail::is_maxarray<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<const F &, std::ptrdiff_t, const Args &...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(const F &f, const adt::irange_t &inds, const Args &... args) {
  std::ptrdiff_t s = inds.shape();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, inds[i], args...);
  return rs;
}

namespace detail {
struct maxarray_iotaMapMulti {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, F &&f, Args &&... args) const {
    return cxx::invoke(std::forward<F>(f), adt::set<adt::index_t<1>>(i),
                       std::forward<Args>(args)...);
  }
};
}

template <
    typename C, std::size_t D, typename F, typename... Args,
    std::enable_if_t<detail::is_maxarray<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<const F &, adt::index_t<D>, const Args &...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(const F &f, const adt::steprange_t<D> &inds,
                const Args &... args) {
  static_assert(D == 1, "");
  return iotaMap<C>(
      detail::maxarray_iotaMapMulti(),
      adt::irange_t(inds.imin()[0], inds.imax()[0], inds.istep()[0]),
      std::forward<F>(f), std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, std::size_t N, typename... Args,
          typename C = adt::maxarray<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::maxarray<T, N> &xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename C = adt::maxarray<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, adt::maxarray<T, N> &&xs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, std::move(xs[i]), args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename T2, std::size_t N2,
          typename... Args, typename C = adt::maxarray<T, N>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::maxarray<T, N> &xs, const adt::maxarray<T2, N2> &ys,
         Args &&... args) {
  std::ptrdiff_t s = xs.size();
  cxx_assert(std::ptrdiff_t(ys.size()) == s);
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename T2, std::size_t N2,
          typename T3, std::size_t N3, typename... Args,
          typename C = adt::maxarray<T, N>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const adt::maxarray<T, N> &xs, const adt::maxarray<T2, N2> &ys,
         const adt::maxarray<T3, N3> &zs, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  cxx_assert(std::ptrdiff_t(ys.size()) == s);
  cxx_assert(std::ptrdiff_t(zs.size()) == s);
  CR rs(s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], zs[i], args...);
  return rs;
}

// fmapStencil

template <typename F, typename G, typename T, std::size_t N, typename BM,
          typename BP, typename... Args, typename C = adt::maxarray<T, N>,
          typename B = std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t>>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const adt::maxarray<T, N> &xs, std::size_t bmask,
               BM &&bm, BP &&bp, Args &&... args) {
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

template <std::size_t D, typename F, typename G, typename T, std::size_t N,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::maxarray<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t>>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::maxarray<T, N> &xs,
                    std::size_t bmask, const std::decay_t<BCB> &bm,
                    const std::decay_t<BCB> &bp, Args &&... args) {
  std::ptrdiff_t s = xs.size();
  CR rs(s);
  R *restrict const rp = rs.data();
  const T *restrict const xp = xs.data();
  if (s == 1) {
    rp[0] = cxx::invoke(std::forward<F>(f), xp[0], bmask, mextract(bm),
                        mextract(bp), std::forward<Args>(args)...);
  } else if (s > 1) {
    rp[0] = cxx::invoke(f, xp[0], bmask & 0b01, mextract(bm),
                        cxx::invoke(g, xp[1], 0), args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rp[i] = cxx::invoke(f, xp[i], 0b00, cxx::invoke(g, xp[i - 1], 1),
                          cxx::invoke(g, xp[i + 1], 0), args...);
    rp[s - 1] =
        cxx::invoke(f, xp[s - 1], bmask & 0b10, cxx::invoke(g, xp[s - 2], 1),
                    mextract(bp), args...);
  }
  return rs;
}

// head, last

template <typename T, std::size_t N>
constexpr const T &head(const adt::maxarray<T, N> &xs) {
  cxx_assert(!xs.empty());
  return xs[0];
}

template <typename T, std::size_t N>
constexpr const T &last(const adt::maxarray<T, N> &xs) {
  cxx_assert(!xs.empty());
  return xs[xs.size() - 1];
}

template <typename T, std::size_t N>
constexpr T &&head(adt::maxarray<T, N> &&xs) {
  cxx_assert(!xs.empty());
  return std::move(xs[0]);
}

template <typename T, std::size_t N>
constexpr T &&last(adt::maxarray<T, N> &&xs) {
  cxx_assert(!xs.empty());
  return std::move(xs[xs.size() - 1]);
}

// boundary

template <typename T, std::size_t N, typename CT = adt::maxarray<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::maxarray<T, N> &xs, std::ptrdiff_t i) {
  cxx_assert(i >= 0 && i < 2);
  return munit<BC>(i == 0 ? head(xs) : last(xs));
}

// boundaryMap

template <typename F, typename T, std::size_t N, typename... Args,
          typename CT = adt::maxarray<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::maxarray<T, N> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// indexing

template <typename T, std::size_t N>
const T &restrict getIndex(const adt::maxarray<T, N> &xs, std::ptrdiff_t i) {
  cxx_assert(i >= 0 && i < std::ptrdiff_t(xs.size()));
  return xs[i];
}

template <typename> class accumulator;
template <typename T, std::size_t N> class accumulator<adt::maxarray<T, N>> {
  adt::maxarray<T, N> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &restrict operator[](std::ptrdiff_t i) { return data[i]; }
  decltype(auto) finalize() { return std::move(data); }
};

// foldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::maxarray<T, N> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  R r(std::forward<Z>(z));
#if 0
#pragma omp declare reduction(red : R : (                                      \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(red : r)
#endif
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, adt::maxarray<T, N> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  R r(std::forward<Z>(z));
#if 0
#pragma omp declare reduction(red : R : (                                      \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(red : r)
#endif
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r),
                    cxx::invoke(f, std::move(xs[i]), args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename T2, std::size_t N2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::maxarray<T, N> &xs,
           const adt::maxarray<T2, N2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  std::ptrdiff_t s = xs.size();
  cxx_assert(std::ptrdiff_t(ys.size()) == s);
  R r(std::forward<Z>(z));
#if 0
#pragma omp declare reduction(red : R : (                                      \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(red : r)
#endif
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], args...));
  return r;
}

// dump

template <typename T, std::size_t N>
ostreamer dump(const adt::maxarray<T, N> &xs) {
  std::ptrdiff_t s = xs.size();
  std::ostringstream os;
  os << "maxarray{";
  for (std::ptrdiff_t i = 0; i < s; ++i)
    os << xs[i] << ",";
  os << "}";
  return ostreamer(os.str());
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_maxarray<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR munit(T &&x) {
  static_assert(fun_traits<CR>::max_size() > 0, "");
  CR rs(1);
  rs[0] = std::forward<T>(x);
  return rs;
}

// mjoin

template <typename T, std::size_t N, std::size_t N2,
          typename CT = adt::maxarray<T, N>>
CT mjoin(const adt::maxarray<adt::maxarray<T, N>, N2> &xss) {
  CT rs;
  std::size_t s = 0;
  for (const auto &xs : xss) {
    std::copy(xs.begin(), xs.end(), rs.end());
    s += xs.size();
  }
  rs.resize(s);
  return rs;
}

template <typename T, std::size_t N, std::size_t N2,
          typename CT = adt::maxarray<T, N>>
CT mjoin(adt::maxarray<adt::maxarray<T, N>, N2> &&xss) {
  CT rs;
  std::size_t s = 0;
  for (const auto &xs : xss) {
    std::move(xs.begin(), xs.end(), rs.end());
    s += xs.size();
  }
  rs.resize(s);
  return rs;
}

// mbind

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const adt::maxarray<T, N> &xs, Args &&... args) {
  static_assert(detail::is_maxarray<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, adt::maxarray<T, N> &&xs, Args &&... args) {
  static_assert(detail::is_maxarray<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T, std::size_t N>
constexpr const T &mextract(const adt::maxarray<T, N> &xs) {
  cxx_assert(!xs.empty());
  return xs[0];
}

template <typename T, std::size_t N>
constexpr T &&mextract(adt::maxarray<T, N> &&xs) {
  cxx_assert(!xs.empty());
  return std::move(xs[0]);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename C = adt::maxarray<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::maxarray<T, N> &xs,
            Args &&... args) {
  return munit<CR>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                           std::forward<Z>(z), xs,
                           std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_maxarray<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR mzero() {
  return CR();
}

// mplus

template <typename T, std::size_t N, typename... Ts, std::size_t... Ns,
          typename CT = adt::maxarray<T, N>>
CT mplus(const adt::maxarray<T, N> &xs, const adt::maxarray<Ts, Ns> &... xss) {
  CT rs;
  for (auto pxs : std::initializer_list<const CT *>{&xs, &xss...}) {
    std::copy(pxs->begin(), pxs->end(), rs.end());
    rs.resize(rs.size() + pxs->size());
  }
  return rs;
}

template <typename T, std::size_t N, typename... Ts, std::size_t... Ns,
          typename CT = adt::maxarray<T, N>>
CT mplus(adt::maxarray<T, N> &&xs, adt::maxarray<Ts, Ns> &&... xss) {
  CT rs;
  for (auto pxs : std::initializer_list<const CT *>{&xs, &xss...}) {
    std::move(pxs->begin(), pxs->end(), rs.end());
    rs.resize(rs.size() + pxs->size());
  }
  return rs;
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_maxarray<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT msome(T &&x, Ts &&... ys) {
  CT rs;
  rs.push_back(std::forward<T>(x));
  std::make_tuple(rs.push_back(std::forward<T>(ys))...);
  return rs;
}

// mempty

template <typename T, std::size_t N>
constexpr bool mempty(const adt::maxarray<T, N> &xs) {
  return xs.empty();
}

// msize

template <typename T, std::size_t N>
constexpr std::size_t msize(const adt::maxarray<T, N> &xs) {
  return xs.size();
}
}

#define FUN_MAXARRAY_HPP_DONE
#endif // #ifdef FUN_MAXARRAY_HPP
#ifndef FUN_MAXARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
