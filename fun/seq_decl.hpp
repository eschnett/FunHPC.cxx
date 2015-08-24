#ifndef FUN_SEQ_DECL_HPP
#define FUN_SEQ_DECL_HPP

#include <adt/seq_decl.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/apply.hpp>
#include <cxx/invoke.hpp>
#include <cxx/tuple.hpp>
#include <fun/fun_decl.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace fun {

// is_seq

namespace detail {
template <typename> struct is_seq : std::false_type {};
template <typename A, typename B, typename T>
struct is_seq<adt::seq<A, B, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename A, typename B, typename T>
struct fun_traits<adt::seq<A, B, T>> {
  template <typename U> using constructor = adt::seq<A, B, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  // static constexpr std::ptrdiff_t rank = fun_traits<A>::rank;
  // static_assert(fun_traits<B>::rank == rank, "");
  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;

  typedef dummy boundary_dummy;

  static constexpr std::size_t min_size() {
    return fun_traits<A>::min_size() + fun_traits<B>::min_size();
  }
  static constexpr std::size_t max_size() {
    return fun_traits<A>::max_size() == std::size_t(-1) ||
                   fun_traits<B>::max_size() == std::size_t(-1)
               ? std::size_t(-1)
               : fun_traits<A>::max_size() + fun_traits<B>::max_size();
  }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_seq<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args);

// fmap

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CT = adt::seq<A, B, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap(F &&f, const adt::seq<A, B, T> &xs, Args &&... args);

template <typename F, typename A, typename B, typename T, typename T2,
          typename... Args, typename CT = adt::seq<A, B, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap2(F &&f, const adt::seq<A, B, T> &xs, const adt::seq<A, B, T2> &ys,
         Args &&... args);

template <typename F, typename A, typename B, typename T, typename T2,
          typename T3, typename... Args, typename CT = adt::seq<A, B, T>,
          typename R = cxx::invoke_of_t<F, T, T2, T3, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmap3(F &&f, const adt::seq<A, B, T> &xs, const adt::seq<A, B, T2> &ys,
         const adt::seq<A, B, T3> &zs, Args &&... args);

// head, last

template <typename A, typename B, typename T>
decltype(auto) head(const adt::seq<A, B, T> &xs);

template <typename A, typename B, typename T>
decltype(auto) last(const adt::seq<A, B, T> &xs);

// foldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs, Args &&... args);

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs,
           const adt::seq<A, B, T2> &ys, Args &&... args);

// dump

template <typename A, typename B, typename T>
ostreamer dump(const adt::seq<A, B, T> &xs);

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_seq<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT munit(T &&x);

#if 0

// mjoin

template <typename A, typename B, typename T, typename CT = adt::seq<A, B, T>>
CT mjoin(const adt::seq<A, B, adt::seq<A, B, T>> &xss);

// mbind

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const adt::seq<A, B, T> &xs, Args &&... args);

#endif

// mextract

template <typename A, typename B, typename T>
decltype(auto) mextract(const adt::seq<A, B, T> &xs);

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args, typename CT = adt::seq<A, B, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs,
            Args &&... args);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_seq<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero();

#if 0

// mplus

template <typename A, typename B, typename T, typename... Ts,
          typename CT = adt::seq<A, B, T>>
CT mplus(const adt::seq<A, B, T> &xs, const adt::seq<A, B, Ts> &... yss);

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_seq<C>::value> * = nullptr,
          typename CT = typename fun_traits<C>::template constructor<T>>
CT msome(T &&x, Ts &&... ys);

#endif

// mempty

template <typename A, typename B, typename T>
bool mempty(const adt::seq<A, B, T> &xs);

// msize

template <typename A, typename B, typename T>
std::size_t msize(const adt::seq<A, B, T> &xs);
}

#define FUN_SEQ_DECL_HPP_DONE
#endif // #ifdef FUN_SEQ_DECL_HPP
#ifndef FUN_SEQ_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
