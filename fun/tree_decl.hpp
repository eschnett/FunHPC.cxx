#ifndef FUN_TREE_DECL_HPP
#define FUN_TREE_DECL_HPP

#include <adt/tree_decl.hpp>

#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <type_traits>

namespace fun {

// is_tree

namespace detail {
template <typename> struct is_tree : std::false_type {};
template <typename A, typename T>
struct is_tree<adt::tree<A, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename A, typename T> struct fun_traits<adt::tree<A, T>> {
  template <typename U> using constructor = adt::tree<A, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  typedef typename fun_traits<A>::index_type index_type;
  typedef adt::tree<typename fun_traits<A>::boundary_dummy, adt::dummy>
      boundary_dummy;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args);

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args);

// fmap

template <typename F, typename A, typename T, typename... Args,
          typename CT = adt::tree<A, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap(F &&f, const adt::tree<A, T> &xs, Args &&... args);

template <typename F, typename A, typename T, typename T2, typename... Args,
          typename CT = adt::tree<A, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
         Args &&... args);

template <typename F, typename A, typename T, typename T2, typename T3,
          typename... Args, typename CT = adt::tree<A, T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap3(F &&f, const adt::tree<A, T> &xs, const adt::tree<A, T2> &ys,
         const adt::tree<A, T3> &zs, Args &&... args);

// head, last

template <typename A, typename T> T head(const adt::tree<A, T> &xs);

template <typename A, typename T> T last(const adt::tree<A, T> &xs);

// boundary

template <typename A, typename T, typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::tree<A, T> &xs, std::ptrdiff_t i);

// boundaryMap

template <typename F, typename A, typename T, typename... Args,
          typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::tree<A, T> &xs, std::ptrdiff_t i,
                Args &&... args);

// fmapStencil

template <typename F, typename G, typename A, typename T, typename BM,
          typename BP, typename... Args, typename CT = adt::tree<A, T>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencil(F &&f, const G &g, const adt::tree<A, T> &xs, std::size_t bmask,
               BM &&bm, BP &&bp, Args &&... args);

template <std::size_t D, typename F, typename G, typename A, typename T,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<A, T> &xs, std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    Args &&... args);

template <std::size_t D, typename F, typename G, typename A, typename T,
          typename... Args, std::enable_if_t<D == 2> * = nullptr,
          typename CT = adt::tree<A, T>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::tree<A, T> &xs, std::size_t bmask,
                    const typename adt::idtype<BCB>::element_type &bm0,
                    const typename adt::idtype<BCB>::element_type &bm1,
                    const typename adt::idtype<BCB>::element_type &bp0,
                    const typename adt::idtype<BCB>::element_type &bp1,
                    Args &&... args);

// foldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args, typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs, Args &&... args);

template <typename F, typename Op, typename Z, typename A, typename T,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs,
           const adt::tree<A, T2> &ys, Args &&... args);

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero();

// mjoin

template <typename A, typename T, typename CT = adt::tree<A, T>>
CT mjoin(const adt::tree<A, adt::tree<A, T>> &xss);

// mbind

template <typename F, typename A, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const adt::tree<A, T> &xs, Args &&... args);

// mextract

template <typename A, typename T>
decltype(auto) mextract(const adt::tree<A, T> &xs);

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          typename... Args, typename C = adt::tree<A, adt::dummy>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::tree<A, T> &xs, Args &&... args);

// mplus

template <typename A, typename T, typename... Ts, typename CT = adt::tree<A, T>>
CT mplus(const adt::tree<A, T> &xss, const adt::tree<A, Ts> &... yss);

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_tree<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT msome(T &&x, Ts &&... ys);

// mempty

template <typename A, typename T> bool mempty(const adt::tree<A, T> &xs);

// msize

template <typename A, typename T> std::size_t msize(const adt::tree<A, T> &xs);
}

#define FUN_TREE_DECL_HPP_DONE
#endif // #ifdef FUN_TREE_DECL_HPP
#ifndef FUN_TREE_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
