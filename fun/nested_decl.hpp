#ifndef FUN_NESTED_DECL_HPP
#define FUN_NESTED_DECL_HPP

#include <adt/nested_decl.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace fun {

// is_nested

namespace detail {
template <typename> struct is_nested : std::false_type {};
template <typename P, typename A, typename T, typename Policy>
struct is_nested<adt::nested<P, A, T, Policy>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename P, typename A, typename T, typename Policy>
struct fun_traits<adt::nested<P, A, T, Policy>> {
  template <typename U>
  using constructor =
      adt::nested<P, A, std::decay_t<U>,
                  typename Policy::template rebind<std::decay_t<U>>::other>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  typedef typename fun_traits<A>::index_type index_type;

  typedef adt::nested<typename fun_traits<P>::boundary_dummy,
                      typename fun_traits<A>::boundary_dummy, adt::dummy,
                      typename Policy::template rebind<adt::dummy>::other>
      boundary_dummy;

  static constexpr std::size_t min_size =
      fun_traits<P>::min_size * fun_traits<A>::min_size;
  static constexpr std::size_t max_size =
      fun_traits<P>::max_size == std::size_t(-1) ||
              fun_traits<A>::max_size == std::size_t(-1)
          ? std::size_t(-1)
          : fun_traits<P>::max_size * fun_traits<A>::max_size;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(const typename CR::policy_type &policy, F &&f,
           const adt::irange_t &inds, Args &&... args);

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  return iotaMap<C>(typename CR::policy_type(), std::forward<F>(f), inds,
                    std::forward<Args>(args)...);
}

template <
    typename C, std::size_t D, typename F, typename... Args,
    std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<F &&, const adt::index_t<D> &, Args &&...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(const typename CR::policy_type &policy, F &&f,
                const adt::range_t<D> &inds, Args &&... args);

template <
    typename C, std::size_t D, typename F, typename... Args,
    std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<F &&, const adt::index_t<D> &, Args &&...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args) {
  return iotaMapMulti<C>(typename CR::policy_type(), std::forward<F>(f), inds,
                         std::forward<Args>(args)...);
}

// fmap

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args, typename C = adt::nested<P, A, T, Policy>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::nested<P, A, T, Policy> &xss, Args &&... args);

template <typename F, typename P, typename A, typename T, typename Policy,
          typename T2, typename Policy2, typename... Args,
          typename C = adt::nested<P, A, T, Policy>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, T2, Policy2> &yss, Args &&... args);

template <typename F, typename P, typename A, typename T, typename Policy,
          typename T2, typename Policy2, typename T3, typename Policy3,
          typename... Args, typename C = adt::nested<P, A, T, Policy>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, T2, Policy2> &yss,
         const adt::nested<P, A, T3, Policy3> &zss, Args &&... args);

// fmapStencil

template <typename F, typename G, typename P, typename A, typename T,
          typename Policy, typename BM, typename BP, typename... Args,
          typename C = adt::nested<P, A, T, Policy>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
               std::size_t bmask, BM &&bm, BP &&bp, Args &&... args);

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 0> * = nullptr,
          typename CT = adt::nested<P, A, T, Policy>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, Args &&... args);

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::nested<P, A, T, Policy>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bp0, Args &&... args);

template <std::size_t D, typename F, typename G, typename P, typename A,
          typename T, typename Policy, typename... Args,
          std::enable_if_t<D == 2> * = nullptr,
          typename CT = adt::nested<P, A, T, Policy>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T, Policy> &xss,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bm1, const std::decay_t<BCB> &bp0,
                    const std::decay_t<BCB> &bp1, Args &&... args);

// head, last

template <typename P, typename A, typename T, typename Policy>
decltype(auto) head(const adt::nested<P, A, T, Policy> &xss);

template <typename P, typename A, typename T, typename Policy>
decltype(auto) last(const adt::nested<P, A, T, Policy> &xss);

// boundary

template <typename P, typename A, typename T, typename Policy,
          typename CT = adt::nested<P, A, T, Policy>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::nested<P, A, T, Policy> &xs, std::ptrdiff_t i);

// boudaryMap

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args, typename CT = adt::nested<P, A, T, Policy>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::nested<P, A, T, Policy> &xs, std::ptrdiff_t i,
                Args &&... args);

// indexing

template <typename P, typename A, typename T, typename Policy>
decltype(auto) getIndex(const adt::nested<P, A, T, Policy> &xs,
                        std::ptrdiff_t i);

template <typename> class accumulator;
template <typename P, typename A, typename T, typename Policy>
class accumulator<adt::nested<P, A, T, Policy>> {
  typedef adt::nested<P, A, T, Policy> CT;
  typedef typename fun_traits<CT>::template constructor<T> AT;
  accumulator<AT> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  auto finalize() -> CT;
};

// foldMap

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename Policy, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
          Args &&... args);

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename Policy, typename T2, typename Policy2,
          typename... Args, typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
           const adt::nested<P, A, T2, Policy2> &yss, Args &&... args);

// dump

template <typename P, typename A, typename T, typename Policy>
ostreamer dump(const adt::nested<P, A, T, Policy> &xss);

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(const typename CT::policy_type &policy, T &&x);

template <typename C, typename T,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x) {
  return munit<C>(typename CT::policy_type(), std::forward<T>(x));
}

// mjoin

template <typename P, typename A, typename T, typename Policy, typename Policy2,
          typename CT = adt::nested<P, A, T, Policy>>
CT mjoin(const adt::nested<P, A, adt::nested<P, A, T, Policy>, Policy2> &xssss);

// mbind

template <typename F, typename P, typename A, typename T, typename Policy,
          typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const adt::nested<P, A, T, Policy> &xss, Args &&... args);

// mextract

template <typename P, typename A, typename T, typename Policy>
decltype(auto) mextract(const adt::nested<P, A, T, Policy> &xss);

// mfoldMap

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename Policy, typename... Args,
          typename C = adt::nested<P, A, T, Policy>,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T, Policy> &xss,
            Args &&... args);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero(const typename CR::policy_type &policy);

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return mzero<C, R>(typename CR::policy_type());
}

// mplus

template <typename P, typename A, typename T, typename Policy, typename... Ts,
          typename... Policies, typename CT = adt::nested<P, A, T, Policy>>
CT mplus(const adt::nested<P, A, T, Policy> &xss,
         const adt::nested<P, A, Ts, Policies> &... yss);

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT msome(T &&x, Ts &&... ys);

// mempty

template <typename P, typename A, typename T, typename Policy>
bool mempty(const adt::nested<P, A, T, Policy> &xss);

// msize

template <typename P, typename A, typename T, typename Policy>
std::size_t msize(const adt::nested<P, A, T, Policy> &xss);
}

#define FUN_NESTED_DECL_HPP_DONE
#endif // #ifdef FUN_NESTED_DECL_HPP
#ifndef FUN_NESTED_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
