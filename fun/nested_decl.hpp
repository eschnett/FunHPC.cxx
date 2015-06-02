#ifndef FUN_NESTED_DECL_HPP
#define FUN_NESTED_DECL_HPP

#include <adt/nested_decl.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <adt/idtype.hpp>
#include <cxx/invoke.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <tuple>
#include <type_traits>
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
  template <typename U> using constructor = adt::nested<P, A, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  typedef typename fun_traits<A>::index_type index_type;
  typedef adt::nested<P, typename fun_traits<A>::boundary_dummy, adt::dummy>
      boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args);

template <
    typename C, std::size_t D, typename F, typename... Args,
    std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<F &&, const adt::index_t<D> &, Args &&...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args);

// fmap

template <typename F, typename P, typename A, typename T, typename... Args,
          typename C = adt::nested<P, A, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::nested<P, A, T> &xss, Args &&... args);

template <typename F, typename P, typename A, typename T, typename T2,
          typename... Args, typename C = adt::nested<P, A, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::nested<P, A, T> &xss,
         const adt::nested<P, A, T2> &yss, Args &&... args);

template <typename F, typename P, typename A, typename T, typename T2,
          typename T3, typename... Args, typename C = adt::nested<P, A, T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap3(F &&f, const adt::nested<P, A, T> &xss,
         const adt::nested<P, A, T2> &yss, const adt::nested<P, A, T3> &zss,
         Args &&... args);

// fmapStencil

template <typename F, typename G, typename P, typename A, typename T,
          typename BM, typename BP, typename... Args,
          typename C = adt::nested<P, A, T>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const adt::nested<P, A, T> &xss, std::size_t bmask,
               BM &&bm, BP &&bp, Args &&... args);

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 0> * = nullptr,
          typename CT = adt::nested<P, A, T>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask, Args &&... args);

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::nested<P, A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    Args &&... args);

template <std::ptrdiff_t D, typename F, typename G, typename P, typename A,
          typename T, typename... Args, std::enable_if_t<D == 2> * = nullptr,
          typename CT = adt::nested<P, A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::nested<P, A, T> &xss,
                    std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bm1,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    const typename adt::idtype<BCB>::element_type &bp1,
                    Args &&... args);

// head, last

template <typename P, typename A, typename T>
decltype(auto) head(const adt::nested<P, A, T> &xss);

template <typename P, typename A, typename T>
decltype(auto) last(const adt::nested<P, A, T> &xss);

// boundary

template <typename P, typename A, typename T,
          typename CT = adt::nested<P, A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::nested<P, A, T> &xs, std::ptrdiff_t i);

// boudaryMap

template <typename F, typename P, typename A, typename T, typename... Args,
          typename CT = adt::nested<P, A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::nested<P, A, T> &xs, std::ptrdiff_t i,
                Args &&... args);

// indexing

template <typename P, typename A, typename T>
decltype(auto) getIndex(const adt::nested<P, A, T> &xs, std::ptrdiff_t i);

template <typename> class accumulator;
template <typename P, typename A, typename T>
class accumulator<adt::nested<P, A, T>> {
  typedef typename fun_traits<adt::nested<P, A, T>>::template constructor<T> AT;
  accumulator<AT> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  adt::nested<P, A, T> finalize();
};

// foldMap

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
          Args &&... args);

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
           const adt::nested<P, A, T2> &yss, Args &&... args);

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x);

// mjoin

template <typename P, typename A, typename T,
          typename CT = adt::nested<P, A, T>>
CT mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss);

// mbind

template <typename F, typename P, typename A, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const adt::nested<P, A, T> &xss, Args &&... args);

// mextract

template <typename P, typename A, typename T>
decltype(auto) mextract(const adt::nested<P, A, T> &xss);

// mfoldMap

template <typename F, typename Op, typename Z, typename P, typename A,
          typename T, typename... Args, typename C = adt::nested<P, A, T>,
          typename R = cxx::invoke_of_t<F &&, T, Args &&...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::nested<P, A, T> &xss,
            Args &&... args);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero();

// mplus

template <typename P, typename A, typename T, typename... Ts,
          typename CT = adt::nested<P, A, T>>
CT mplus(const adt::nested<P, A, T> &xss, const adt::nested<P, A, Ts> &... yss);

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_nested<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT msome(T &&x, Ts &&... ys);

// mempty

template <typename P, typename A, typename T>
bool mempty(const adt::nested<P, A, T> &xss);

// msize

template <typename P, typename A, typename T>
std::size_t msize(const adt::nested<P, A, T> &xss);
}

#define FUN_NESTED_DECL_HPP_DONE
#endif // #ifdef FUN_NESTED_DECL_HPP
#ifndef FUN_NESTED_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
