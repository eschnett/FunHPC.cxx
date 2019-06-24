#ifndef FUN_SHARED_FUTURE_HPP
#define FUN_SHARED_FUTURE_HPP

#include <qthread/future.hpp>

#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_shared_future

namespace detail {
template <typename> struct is_shared_future : std::false_type {};
template <typename T>
struct is_shared_future<qthread::shared_future<T>> : std::true_type {};
} // namespace detail

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<qthread::shared_future<T>> {
  template <typename U>
  using constructor = qthread::shared_future<std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;

  typedef dummy boundary_dummy;

  // We don't want empty shared_futures since they don't support mjoin
  static constexpr std::size_t min_size() { return 1; }
  static constexpr std::size_t max_size() { return 1; }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  std::size_t s = inds.size();
  cxx_assert(s <= 1);
  if (__builtin_expect(s == 0, false))
    return CR();
  return qthread::async(std::forward<F>(f), inds[0],
                        std::forward<Args>(args)...)
      .share();
}

// iotaMapMulti

template <
    typename C, std::size_t D, typename F, typename... Args,
    std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
    typename R = std::decay_t<cxx::invoke_of_t<F, adt::index_t<D>, Args...>>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::steprange_t<D> &inds, Args &&... args) {
  std::size_t s = inds.size();
  cxx_assert(s <= 1);
  if (__builtin_expect(s == 0, false))
    return CR();
  return qthread::async(std::forward<F>(f), inds.imin(),
                        std::forward<Args>(args)...)
      .share();
}

// fmap

template <typename F, typename T, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  bool s = xs.valid();
  if (!s)
    return CR();
  return xs
      .then([f = std::forward<F>(f),
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), std::move(args)...);
      })
      .share();
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const qthread::shared_future<T> &xs,
         const qthread::shared_future<T2> &ys, Args &&... args) {
  bool s = xs.valid();
  cxx_assert(ys.valid() == s);
  if (!s)
    return CR();
  return xs
      .then([f = std::forward<F>(f), ys,
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), ys.get(),
                           std::move(args)...);
      })
      .share();
}

template <typename F, typename T, typename T2, typename T3, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const qthread::shared_future<T> &xs,
         const qthread::shared_future<T2> &ys,
         const qthread::shared_future<T3> &zs, Args &&... args) {
  bool s = xs.valid();
  cxx_assert(ys.valid() == s);
  cxx_assert(zs.valid() == s);
  if (!s)
    return CR();
  return xs
      .then([f = std::forward<F>(f), ys, zs,
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), ys.get(), zs.get(),
                           std::move(args)...);
      })
      .share();
}

// fmapStencil

template <typename F, typename G, typename T, typename BM, typename BP,
          typename... Args, typename CT = qthread::shared_future<T>,
          typename B = std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t>>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const qthread::shared_future<T> &xs,
               std::size_t bmask, BM &&bm, BP &&bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  bool s = xs.valid();
  if (__builtin_expect(!s, false))
    return CR();
  return xs
      .then([f = std::forward<F>(f), bmask, bm = std::forward<BM>(bm),
             bp = std::forward<BP>(bp),
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), bmask, std::move(bm),
                           std::move(bp), std::move(args)...);
      })
      .share();
}

// fmapStencilMulti

template <std::size_t D, typename F, typename G, typename T, typename... Args,
          std::enable_if_t<D == 1> * = nullptr,
          typename CT = qthread::shared_future<T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t>>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const qthread::shared_future<T> &xs,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bp0, Args &&... args) {
  bool s = xs.valid();
  if (__builtin_expect(!s, false))
    return CR();
  return xs
      .then([f = std::forward<F>(f), bmask, bm0, bp0,
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), bmask, std::move(bm0).get(),
                           std::move(bp0).get(), std::move(args)...);
      })
      .share();
}

template <std::size_t D, typename F, typename G, typename T, typename... Args,
          std::enable_if_t<D == 2> * = nullptr,
          typename CT = qthread::shared_future<T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = std::decay_t<cxx::invoke_of_t<G, T, std::ptrdiff_t>>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const qthread::shared_future<T> &xs,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bm1, const std::decay_t<BCB> &bp0,
                    const std::decay_t<BCB> &bp1, Args &&... args) {
  bool s = xs.valid();
  if (__builtin_expect(!s, false))
    return CR();
  return xs
      .then([f = std::forward<F>(f), bmask, bm0, bm1, bp0, bp1,
             args...](const qthread::shared_future<T> &xs) mutable {
        return cxx::invoke(std::move(f), xs.get(), bmask, std::move(bm0).get(),
                           std::move(bm1).get(), std::move(bp0).get(),
                           std::move(bp1).get(), std::move(args)...);
      })
      .share();
}

// boundary

template <typename T, typename CT = qthread::shared_future<T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const qthread::shared_future<T> &xs, std::ptrdiff_t i) {
  return xs;
}

// boundaryMap

template <typename F, typename T, typename... Args,
          typename CT = qthread::shared_future<T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const qthread::shared_future<T> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), xs, i, std::forward<Args>(args)...);
}

// head, last

template <typename T> const T &head(const qthread::shared_future<T> &xs) {
  cxx_assert(xs.valid());
  return xs.get();
}

template <typename T> const T &last(const qthread::shared_future<T> &xs) {
  cxx_assert(xs.valid());
  return xs.get();
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
           const qthread::shared_future<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  cxx_assert(ys.valid() == s);
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                     std::forward<Args>(args)...);
}

// dump

template <typename T> ostreamer dump(const qthread::shared_future<T> &xs) {
  bool s = xs.valid();
  if (!s)
    return ostreamer("shared_future{}");
  return ostreamer("shared_future{") +
         foldMap([](const auto &x) { return ostreamer(x); },
                 combine_ostreamers(), ostreamer(), xs) +
         ostreamer("}");
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x) {
  return qthread::make_ready_future(std::forward<T>(x)).share();
}

// mjoin

template <typename T, typename CT = qthread::shared_future<T>>
CT mjoin(const qthread::shared_future<qthread::shared_future<T>> &xss) {
  cxx_assert(xss.valid());
  // return qthread::async([xss]() { return xss.get().get(); }).share();
  return xss.unwrap().share();
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  static_assert(detail::is_shared_future<CR>::value, "");
  cxx_assert(xs.valid());
  // return cxx::invoke(std::forward<F>(f), xs.get(),
  // std::forward<Args>(args)...);
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename T> const T &mextract(const qthread::shared_future<T> &xs) {
  cxx_assert(xs.valid());
  return xs.get();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
            Args &&... args) {
  return qthread::async(
             [](auto &&f, auto &&op, auto &&z, auto &&xs, auto &&... args) {
               return foldMap(std::forward<F>(f), std::forward<Op>(op),
                              std::forward<Z>(z), xs,
                              std::forward<Args>(args)...);
             },
             std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
             std::forward<Args>(args)...)
      .share();
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return qthread::shared_future<R>();
}

// mempty

template <typename T>
constexpr bool mempty(const qthread::shared_future<T> &xs) {
  return !xs.valid();
}

// msize

template <typename T>
constexpr std::size_t msize(const qthread::shared_future<T> &xs) {
  return !mempty(xs);
}
} // namespace fun

#define FUN_SHARED_FUTURE_HPP_DONE
#endif // #ifdef FUN_SHARED_FUTURE_HPP
#ifndef FUN_SHARED_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
