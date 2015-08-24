#ifndef FUN_PAIR_HPP
#define FUN_PAIR_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <utility>

namespace fun {

// is_pair

namespace detail {
template <typename> struct is_pair : std::false_type {};
template <typename L, typename R>
struct is_pair<std::pair<L, R>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, typename L> struct fun_traits<std::pair<L, T>> {
  template <typename U> using constructor = std::pair<L, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;

  typedef dummy boundary_dummy;

  static constexpr std::size_t min_size = 1;
  static constexpr std::size_t max_size = 1;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_pair<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  typedef typename C::first_type L;
  cxx_assert(inds.size() == 1);
  return std::pair<L, R>(L(), cxx::invoke(std::forward<F>(f), inds[0],
                                          std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename L, typename... Args,
          typename C = std::pair<L, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const std::pair<L, T> &xs, Args &&... args) {
  return std::pair<L, R>(xs.first, cxx::invoke(std::forward<F>(f), xs.second,
                                               std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename... Args,
          typename C = std::pair<L, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, std::pair<L, T> &&xs, Args &&... args) {
  return std::pair<L, R>(std::move(xs.left),
                         cxx::invoke(std::forward<F>(f), std::move(xs.second),
                                     std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename T2, typename L2,
          typename... Args, typename C = std::pair<L, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const std::pair<L, T> &xs, const std::pair<L2, T2> &ys,
         Args &&... args) {
  return std::pair<L, R>(xs.first,
                         cxx::invoke(std::forward<F>(f), xs.second, ys.second,
                                     std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.second,
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, std::pair<L, T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.second),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename T2, typename L2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs,
           const std::pair<L2, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.second, ys.second,
                     std::forward<Args>(args)...);
}

// dump

template <typename T, typename L> ostreamer dump(const std::pair<L, T> &xs) {
  std::ostringstream os;
  os << "pair{(" << xs.first << ")," << xs.second << "}";
  return ostreamer(os.str());
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_pair<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR munit(T &&x) {
  typedef typename C::first_type L;
  return std::pair<L, R>(L(), std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename L, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const std::pair<L, T> &xs, Args &&... args) {
  static_assert(detail::is_pair<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), xs.second,
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename L, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, std::pair<L, T> &&xs, Args &&... args) {
  static_assert(detail::is_pair<CR>::value, "");
  return cxx::invoke(std::forward<F>(f), std::move(xs.second),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename L, typename L2, typename CT = std::pair<L, T>>
constexpr CT mjoin(const std::pair<L2, std::pair<L, T>> &xss) {
  return xss.second;
}

template <typename T, typename L, typename L2, typename CT = std::pair<L, T>>
constexpr CT mjoin(std::pair<L2, std::pair<L, T>> &&xss) {
  return std::move(xss.second);
}

// mextract

template <typename T, typename L>
constexpr const T &mextract(const std::pair<L, T> &xs) {
  return xs.second;
}

template <typename T, typename L> constexpr T &&mextract(std::pair<L, T> &&xs) {
  return std::move(xs.second);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>,
          typename C = std::pair<L, T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const std::pair<L, T> &xs, Args &&... args) {
  return std::pair<L, R>(
      xs.first, foldMap(std::forward<F>(f), std::forward<Op>(op),
                        std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mempty

template <typename T, typename L>
constexpr bool mempty(const std::pair<L, T> &xs) {
  return false;
}

// msize

template <typename T, typename L>
constexpr std::size_t msize(const std::pair<L, T> &xs) {
  return 1;
}
}

#define FUN_PAIR_HPP_DONE
#endif // #ifdef FUN_PAIR_HPP
#ifndef FUN_PAIR_HPP_DONE
#error "Cyclic include dependency"
#endif
