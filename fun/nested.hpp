#ifndef FUN_NESTED_HPP
#define FUN_NESTED_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <adt/nested.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <type_traits>
#include <tuple>
#include <utility>

namespace fun {

// is_nested

namespace detail {
template <typename> struct is_nested : std::false_type {};
template <typename P, typename A, typename T>
struct is_nested<adt::nested<P, A, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename P, typename A, typename T>
struct fun_traits<adt::nested<P, A, T>> {
  template <typename U> using constructor = adt::nested<P, A, U>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  typedef typename fun_traits<A>::index_type index_type;
  typedef adt::nested<P, typename fun_traits<A>::boundary_dummy, adt::dummy>
      boundary_dummy;
};

// iotaMap

namespace detail {
template <typename A> struct nested_iotaMap : std::tuple<> {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, std::ptrdiff_t s, F &&f,
                  Args &&... args) const {
    return iotaMap<A>(std::forward<F>(f), s, std::forward<Args>(args)...);
  }
};
}

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr>
auto iotaMap(F &&f, std::ptrdiff_t s, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CR{iotaMap<P>(detail::nested_iotaMap<A>(), std::ptrdiff_t(1), s,
                       std::forward<F>(f), std::forward<Args>(args)...)};
}

namespace detail {
template <typename A> struct nested_iotaMap1 : std::tuple<> {
  template <std::size_t D, typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, const adt::index_t<D> &s, F &&f,
                  Args &&... args) const {
    return iotaMap<A>(std::forward<F>(f), s, std::forward<Args>(args)...);
  }
};
}

template <typename C, typename F, std::size_t D, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr>
auto iotaMap(F &&f, const adt::index_t<D> &s, Args &&... args) {
  typedef std::decay_t<cxx::invoke_of_t<std::decay_t<F>, adt::index_t<D>,
                                        std::decay_t<Args>...>> R;
  typedef typename fun_traits<C>::template constructor<R> CR;
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  return CR{iotaMap<P>(detail::nested_iotaMap1<A>(), std::ptrdiff_t(1), s,
                       std::forward<F>(f), std::forward<Args>(args)...)};
}

// fmap

namespace detail {
struct nested_fmap : std::tuple<> {
  template <typename AT, typename F, typename... Args>
  auto operator()(const AT &xs, F &&f, Args &&... args) const {
    return fmap(f, xs, args...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
adt::nested<P, A, R> fmap(F &&f, const adt::nested<P, A, T> &xss,
                          Args &&... args) {
  return {fmap(detail::nested_fmap(), xss.data, std::forward<F>(f),
               std::forward<Args>(args)...)};
}

namespace detail {
struct nested_fmap2 : std::tuple<> {
  template <typename AT, typename AT2, typename F, typename... Args>
  auto operator()(const AT &xs, const AT2 &ys, F &&f, Args &&... args) const {
    return fmap2(f, xs, ys, args...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
adt::nested<P, A, R> fmap2(F &&f, const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, T2> &yss, Args &&... args) {
  return {fmap2(detail::nested_fmap2(), xss.data, yss.data, std::forward<F>(f),
                std::forward<Args>(args)...)};
}

// fmapStencil

namespace detail {
struct nested_fmapStencil : std::tuple<> {
  template <typename AT, typename F, typename G, typename BM, typename BP,
            typename... Args>
  auto operator()(const AT &xs, F &&f, G &&g, BM &&bm, BP &&bp,
                  Args &&... args) const {
    return fmapStencil(std::forward<F>(f), std::forward<G>(g), xs,
                       std::forward<BM>(bm), std::forward<BP>(bp),
                       std::forward<Args>(args)...);
  }
};
}

template <typename F, typename G, typename P, typename A, typename T,
          typename BM, typename BP, typename... Args,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>>
adt::nested<P, A, R> fmapStencil(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                                 BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  return {fmap(detail::nested_fmapStencil(), xss.data, std::forward<F>(f),
               std::forward<G>(g), std::forward<BM>(bm), std::forward<BP>(bp),
               std::forward<Args>(args)...)};
}

// head, last

namespace detail {
struct nested_head : std::tuple<> {
  template <typename AT> auto operator()(const AT &xs) const {
    return head(xs);
  }
};

struct nested_last : std::tuple<> {
  template <typename AT> auto operator()(const AT &xs) const {
    return last(xs);
  }
};
}

// Note: The call to fmap is necessary in case we use a proxy; calling
// head or mextract on the proxy would copy the whole data structure
// to the local process, which would be prohibitively expensive.

// Note: Why not use foldMap instead of mextract(fmap)?

template <typename P, typename A, typename T>
decltype(auto) head(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_head(), xss.data));
}

template <typename P, typename A, typename T>
decltype(auto) last(const adt::nested<P, A, T> &xss) {
  return mextract(fmap(detail::nested_last(), xss.data));
}

// boundary

namespace detail {
struct nested_boundary {
  template <typename AT> auto operator()(const AT &xs, std::ptrdiff_t i) const {
    return boundary(xs, i);
  }
};
}

template <typename P, typename A, typename T>
auto boundary(const adt::nested<P, A, T> &xs, std::ptrdiff_t i) {
  typedef typename fun_traits<adt::nested<P, A, T>>::boundary_dummy BNPA;
  typedef typename fun_traits<BNPA>::template constructor<T> BNPAT;
  return BNPAT{fmap(detail::nested_boundary(), xs.data, i)};
}

// boudaryMap

template <typename F, typename P, typename A, typename T, typename... Args>
auto boundaryMap(F &&f, const adt::nested<P, A, T> &xs, std::ptrdiff_t i,
                 Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), std::forward<Args>(args)...);
}

// indexing

namespace detail {
struct nested_getIndex : std::tuple<> {
  template <typename AT> auto operator()(const AT &xs, std::ptrdiff_t i) const {
    return xs[i];
  }
};
}

template <typename P, typename A, typename T>
decltype(auto) getIndex(const adt::nested<P, A, T> &xs, std::ptrdiff_t i) {
  return mextract(fmap(detail::nested_getIndex(), xs.data, i));
}

template <typename> class accumulator;
template <typename P, typename A, typename T>
class accumulator<adt::nested<P, A, T>> {
  typedef typename fun_traits<adt::nested<P, A, T>>::template constructor<T> AT;
  accumulator<AT> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  adt::nested<P, A, T> finalize() { return {munit<P>(data.finalize())}; }
};

namespace detail {
struct nested_fmapIndexed : std::tuple<> {
  template <typename AT, typename F, typename... Args>
  auto operator()(const AT &xs, F &&f, std::ptrdiff_t i,
                  Args &&... args) const {
    return fmapIndexed(f, xs, i, args...);
  }
};
}

template <typename F, typename P, typename A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>>
adt::nested<P, A, R> fmapIndexed(F &&f, const adt::nested<P, A, T> &xss,
                                 Args &&... args) {
  return {fmap(detail::nested_fmapIndexed(), xss.data, std::forward<F>(f),
               std::forward<Args>(args)...)};
}

// foldMap

namespace detail {
struct nested_foldMap : std::tuple<> {
  template <typename AT, typename F, typename Op, typename Z, typename... Args>
  auto operator()(const AT &xs, F &&f, Op &&op, Z &&z, Args &&... args) const {
    return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                   xs, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap(detail::nested_foldMap(), op, z, xss.data, std::forward<F>(f),
                 op, z, std::forward<Args>(args)...);
}

namespace detail {
struct nested_foldMap2 : std::tuple<> {
  template <typename AT, typename AT2, typename F, typename Op, typename Z,
            typename... Args>
  auto operator()(const AT &xs, const AT2 &ys, F &&f, Op &&op, Z &&z,
                  Args &&... args) const {
    return foldMap2(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), xs, ys, std::forward<Args>(args)...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
           const adt::nested<P, A, T2> &yss, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap2(detail::nested_foldMap2(), op, z, xss.data, yss.data,
                  std::forward<F>(f), op, z, std::forward<Args>(args)...);
}

// munit

template <typename C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr>
auto munit(T &&x) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR{munit<P>(munit<A>(std::forward<T>(x)))};
}

// mjoin

namespace detail {
struct nested_mextract : std::tuple<> {
  template <typename NPAT> auto operator()(const NPAT &xss) const {
    return mextract(xss.data);
  }
};
struct nested_mjoin : std::tuple<> {
  template <typename ANPAT> auto operator()(const ANPAT &xsss) const {
    return mjoin(fmap(nested_mextract(), xsss));
  }
};
}

template <typename P, typename A, typename T>
adt::nested<P, A, T>
mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss) {
  return {fmap(detail::nested_mjoin(), xssss.data)};
}

// mbind

template <typename F, typename P, typename A, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>,
          typename R = typename CR::value_type>
adt::nested<P, A, R> mbind(F &&f, const adt::nested<P, A, T> &xss,
                           Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xss, std::forward<Args>(args)...));
}

// mextract

template <typename P, typename A, typename T>
decltype(auto) mextract(const adt::nested<P, A, T> &xss) {
  return mextract(mextract(xss.data));
}

// mfoldMap

namespace detail {
struct nested_mfoldMap : std::tuple<> {
  template <typename AT, typename F, typename Op, typename Z, typename... Args>
  auto operator()(const AT &xs, F &&f, Op &&op, Z &&z, Args &&... args) const {
    return mfoldMap(f, op, z, xs, args...);
  }
};
}

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
adt::nested<P, A, R> mfoldMap(F &&f, Op &&op, Z &&z,
                              const adt::nested<P, A, T> &xss,
                              Args &&... args) {
  return {fmap(detail::nested_mfoldMap(), xss.data, std::forward<F>(f),
               std::forward<Op>(op), std::forward<Z>(z),
               std::forward<Args>(args)...)};
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr>
auto mzero() {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  typedef typename fun_traits<A>::template constructor<R> AR;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR{mzero<P, AR>()};
}

// mplus

template <typename P, typename A, typename T, typename... Ts>
adt::nested<P, A, T> mplus(const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, Ts> &... yss) {
  typedef typename adt::nested<P, A, T>::template array_constructor<T> AT;
  return {munit<P, AT>(
      mplus(mempty(xss.data) ? mzero<A, T>() : mextract(xss.data),
            mempty(yss.data) ? mzero<A, T>() : mextract(yss.data)...))};
}

// msome

template <typename C, typename T, typename... Ts, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr>
auto msome(T &&x, Ts &&... ys) {
  typedef typename C::pointer_dummy P;
  typedef typename C::array_dummy A;
  typedef typename fun_traits<C>::template constructor<R> CR;
  return CR{munit<P>(msome<A>(std::forward<T>(x), std::forward<Ts>(ys)...))};
}

// mempty

template <typename P, typename A, typename T>
bool mempty(const adt::nested<P, A, T> &xss) {
  return mempty(xss.data);
}

// msize

template <typename P, typename A, typename T>
std::size_t msize(const adt::nested<P, A, T> &xss) {
  typedef typename adt::nested<P, A, T>::template array_constructor<T> AT;
  return mempty(xss.data) ? 0 : foldMap((std::size_t (*)(const AT &))msize,
                                        std::plus<std::size_t>(), 0, xss.data);
}
}

#define FUN_NESTED_HPP_DONE
#endif // #ifdef FUN_NESTED_HPP
#ifndef FUN_NESTED_HPP_DONE
#error "Cyclic include dependency"
#endif
