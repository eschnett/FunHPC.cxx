#ifndef FUN_GRID_DECL_HPP
#define FUN_GRID_DECL_HPP

#include <adt/grid_decl.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <type_traits>

namespace fun {

// is_grid

namespace detail {
template <typename> struct is_grid : std::false_type {};
template <typename C, typename T, std::size_t D>
struct is_grid<adt::grid<C, T, D>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename C, typename T, std::size_t D>
struct fun_traits<adt::grid<C, T, D>> {
  template <typename U> using constructor = adt::grid<C, std::decay_t<U>, D>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = D;
  typedef typename adt::grid<C, T, D>::index_type index_type;

  typedef adt::grid<C, adt::dummy, D - 1> boundary_dummy;

  static constexpr std::size_t min_size = fun_traits<C>::min_size;
  static constexpr std::size_t max_size = fun_traits<C>::max_size;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args);

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, const adt::range_t<D> &inds, Args &&... args);

// fmap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          typename CT = adt::grid<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap(F &&f, const adt::grid<C, T, D> &xs, Args &&... args);

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename... Args, typename CT = adt::grid<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
         Args &&... args);

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename T3, typename... Args, typename CT = adt::grid<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap3(F &&f, const adt::grid<C, T, D> &xs, const adt::grid<C, T2, D> &ys,
         const adt::grid<C, T3, D> &zs, Args &&... args);

// fmapStencil

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 0> * = nullptr,
          typename CT = adt::grid<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, Args &&... args);

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = adt::grid<C, T, D>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, const std::decay_t<BCB> &bm0,
                    const std::decay_t<BCB> &bp0, Args &&... args);

template <std::size_t D, typename F, typename G, typename C, typename T,
          typename... Args, std::enable_if_t<D == 2> * = nullptr,
          typename CT = adt::grid<C, T, D>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const adt::grid<C, T, D> &xs,
                    std::size_t bmask, const BCB &bm0, const BCB &bm1,
                    const BCB &bp0, const BCB &bp1, Args &&... args);

// head, last

template <typename C, typename T, std::size_t D,
          std::enable_if_t<D == 1> * = nullptr>
decltype(auto) head(const adt::grid<C, T, D> &xs);

template <typename C, typename T, std::size_t D,
          std::enable_if_t<D == 1> * = nullptr>
decltype(auto) last(const adt::grid<C, T, D> &xs);

// boundary

template <typename C, typename T, std::size_t D,
          std::enable_if_t<D != 0> * = nullptr,
          typename CT = adt::grid<C, T, D>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const adt::grid<C, T, D> &xs, std::ptrdiff_t i);

// boundaryMap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          std::enable_if_t<D != 0> * = nullptr,
          typename CT = adt::grid<C, T, D>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const adt::grid<C, T, D> &xs, std::ptrdiff_t i,
                Args &&... args);

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs, Args &&... args);

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::grid<C, T, D> &xs,
           const adt::grid<C, T2, D> &ys, Args &&... args);

// dump

template <typename C, typename T, std::size_t D>
ostreamer dump(const adt::grid<C, T, D> &xs);

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x);

// // mjoin
//
// template <typename C, typename T>
// adt::grid<C, T> mjoin(const adt::grid<C, adt::grid<C, T>> &xss) {
//   return adt::grid<C, T>(typename adt::grid<C, T>::join(), xss);
// }
//
// // mbind
//
// template <typename F, typename C, typename T, typename... Args,
//           typename CR = cxx::invoke_of_t<F, T, Args...>,
//           typename R = typename CR::value_type>
// adt::grid<C, R> mbind(F &&f, const adt::grid<C, T> &xs, Args &&... args) {
//   return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
// }

// mextract

template <typename C, typename T, std::size_t D>
decltype(auto) mextract(const adt::grid<C, T, D> &xs);

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename T,
          std::size_t D, typename... Args, typename C = adt::grid<A, T, D>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::grid<A, T, D> &xs,
            Args &&... args);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_grid<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero();

// // mplus
//
// template <typename C, typename T, typename... Ts>
// adt::grid<C, T> mplus(const adt::grid<C, T> &xss,
//                       const adt::grid<C, Ts> &... yss) {
//   return adt::grid<C, T>(xss, yss...);
// }
//
// // msome
//
// template <typename C, typename T, typename... Ts,
//           typename R = std::decay_t<T>,
//           std::enable_if_t<detail::is_grid<C<R>>::value> * = nullptr>
// C<R> msome(T &&x, Ts &&... ys) {
//   return C<R>{std::forward<T>(x), std::forward<Ts>(ys)...};
// }

// mempty

template <typename C, typename T, std::size_t D>
bool mempty(const adt::grid<C, T, D> &xs);

// msize

template <typename C, typename T, std::size_t D>
std::size_t msize(const adt::grid<C, T, D> &xs);
}

#define FUN_GRID_DECL_HPP_DONE
#endif // #ifdef FUN_GRID_DECL_HPP
#ifndef FUN_GRID_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
