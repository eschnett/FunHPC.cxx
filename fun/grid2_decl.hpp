#ifndef FUN_GRID2_DECL_HPP
#define FUN_GRID2_DECL_HPP

#include <adt/grid2_decl.hpp>

#include <adt/index.hpp>
#include <cxx/invoke.hpp>

#include <type_traits>

namespace fun {

// is_grid2

namespace detail {
template <typename> struct is_grid2 : std::false_type {};
template <typename C, typename T, std::size_t D>
struct is_grid2<adt::grid2<C, T, D>> : std::true_type {};
} // namespace detail

// traits

template <typename> struct fun_traits;
template <typename C, typename T, std::size_t D>
struct fun_traits<adt::grid2<C, T, D>> {
  template <typename U> using constructor = adt::grid2<C, std::decay_t<U>, D>;
  // typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = D;
  typedef typename adt::grid2<C, T, D>::index_type index_type;

  static constexpr std::size_t min_size() { return fun_traits<C>::min_size(); }
  static constexpr std::size_t max_size() { return fun_traits<C>::max_size(); }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_grid2<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args);

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_grid2<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::range_t<D> &inds, Args &&... args);

// fmap

template <typename F, typename C, typename T, std::size_t D, typename... Args,
          typename CT = adt::grid2<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap(F &&f, const adt::grid2<C, T, D> &xs, Args &&... args);

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename... Args, typename CT = adt::grid2<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::grid2<C, T, D> &xs, const adt::grid2<C, T2, D> &ys,
         Args &&... args);

template <typename F, typename C, typename T, std::size_t D, typename T2,
          typename T3, typename... Args, typename CT = adt::grid2<C, T, D>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap3(F &&f, const adt::grid2<C, T, D> &xs, const adt::grid2<C, T2, D> &ys,
         const adt::grid2<C, T3, D> &zs, Args &&... args);

// boundary

template <typename C, typename T, std::size_t D,
          typename CT = adt::grid2<C, T, D>>
CT boundary(const adt::grid2<C, T, D> &xs, std::ptrdiff_t f, std::ptrdiff_t d);

// fmapStencil

template <typename F, typename G, typename C, typename T, std::size_t D,
          typename B, typename... Args, typename CT = adt::grid2<C, T, D>,
          typename R =
              cxx::invoke_of_t<F, T, std::array<std::array<B, D>, 2>, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const adt::grid2<C, T, D> &xs,
               const std::array<std::array<adt::grid2<C, B, D>, D>, 2> &bss,
               Args &&... args);

// foldMap

template <typename F, typename Op, typename Z, typename C, typename T,
          std::size_t D, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::grid2<C, T, D> &xs,
          Args &&... args);
} // namespace fun

#define FUN_GRID2_DECL_HPP_DONE
#endif // #ifdef FUN_GRID2_DECL_HPP
#ifndef FUN_GRID2_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
