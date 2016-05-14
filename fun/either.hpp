#ifndef FUN_EITHER_HPP
#define FUN_EITHER_HPP

#include <adt/either.hpp>

#include <adt/dummy.hpp>
#include <adt/index.hpp>
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

// is_either

namespace detail {
template <typename> struct is_either : std::false_type {};
template <typename L, typename R>
struct is_either<adt::either<L, R>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename L, typename R> struct fun_traits<adt::either<L, R>> {
  template <typename U> using constructor = adt::either<L, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef R value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;

  typedef dummy boundary_dummy;

  static constexpr std::size_t min_size() { return 0; }
  static constexpr std::size_t max_size() { return 1; }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  cxx_assert(inds.size() <= 1);
  if (inds.empty())
    return CR::make_left();
  return CR::make_right(
      cxx::invoke(std::forward<F>(f), inds[0], std::forward<Args>(args)...));
}

// fmap

template <typename F, typename T, typename L, typename... Args,
          typename C = adt::either<L, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  bool s = xs.right();
  if (!s)
    return CR::make_left(xs.get_left());
  return CR::make_right(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                    std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename... Args,
          typename C = adt::either<L, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  bool s = xs.right();
  if (!s)
    return CR::make_left(xs.get_left());
  return CR::make_right(cxx::invoke(std::forward<F>(f),
                                    std::move(xs.get_right()),
                                    std::forward<Args>(args)...));
}

template <typename F, typename T, typename L, typename T2, typename L2,
          typename... Args, typename C = adt::either<L, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::either<L, T> &xs, const adt::either<L2, T2> &ys,
         Args &&... args) {
  bool s = xs.right();
  cxx_assert(ys.right() == s);
  if (!s)
    return CR::make_left(xs.get_left());
  return CR::make_right(cxx::invoke(std::forward<F>(f), xs.get_right(),
                                    ys.get_right(),
                                    std::forward<Args>(args)...));
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_right(),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, adt::either<L, T> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_right()),
                     std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename L,
          typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs,
           const adt::either<L, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.right();
  cxx_assert(ys.right() == s);
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get_right(), ys.get_right(),
                     std::forward<Args>(args)...);
}

// dump

template <typename T, typename L> ostreamer dump(const adt::either<L, T> &xs) {
  bool s = xs.right();
  std::ostringstream os;
  os << "either{";
  if (!s)
    os << "left{" << xs.get_left() << "}";
  else
    os << "right{" << xs.get_right() << "}";
  os << "}";
  return ostreamer(os.str());
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return CR::make_right(std::forward<T>(x));
}

// mbind

template <typename F, typename T, typename L, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
CR mbind(F &&f, const adt::either<L, T> &xs, Args &&... args) {
  static_assert(detail::is_either<CR>::value, "");
  if (!xs.right())
    return CR::make_left();
  return cxx::invoke(std::forward<F>(f), xs.get_right(),
                     std::forward<Args>(args)...);
}

template <typename F, typename T, typename L, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
auto mbind(F &&f, adt::either<L, T> &&xs, Args &&... args) {
  static_assert(detail::is_either<CR>::value, "");
  if (!xs.right())
    return std::move(xs);
  return cxx::invoke(std::forward<F>(f), std::move(xs.get_right()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T, typename L, typename CT = adt::either<L, T>>
CT mjoin(const adt::either<L, adt::either<L, T>> &xss) {
  if (!xss.right())
    return CT::make_left(xss.get_left());
  return xss.get_right();
}

template <typename T, typename L, typename CT = adt::either<L, T>>
CT mjoin(adt::either<L, adt::either<L, T>> &&xss) {
  if (!xss.right())
    return CT::make_left(std::move(xss.get_left()));
  return std::move(xss.get_right());
}

// mextract

template <typename T, typename L>
const T &mextract(const adt::either<L, T> &xs) {
  cxx_assert(xs.right());
  return xs.get_right();
}

template <typename T, typename L> T &&mextract(adt::either<L, T> &&xs) {
  cxx_assert(xs.right());
  return std::move(xs.get_right());
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename L,
          typename... Args, typename C = adt::either<L, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::either<L, T> &xs,
            Args &&... args) {
  return munit<C>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                          std::forward<Z>(z), xs, std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_either<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return CR::make_left();
}

// mplus

template <typename T, typename L, typename... Ts,
          typename CT = adt::either<L, T>>
CT mplus(const adt::either<L, T> &xs, const adt::either<L, Ts> &... yss) {
  if (xs.right())
    return xs;
  for (auto pys : std::initializer_list<const adt::either<L, T> *>{&yss...})
    if ((*pys).right())
      return *pys;
  return CT::make_left();
}

template <typename T, typename L, typename... Ts,
          typename CT = adt::either<L, T>>
CT mplus(adt::either<L, T> &&xs, adt::either<L, Ts> &&... yss) {
  if (xs.right())
    return std::move(xs);
  for (auto pys : std::initializer_list<adt::either<L, T> *>{&yss...})
    if ((*pys).right())
      return std::move(*pys);
  return CT::make_left();
}

// mempty

template <typename T, typename L> bool mempty(const adt::either<L, T> &xs) {
  return !xs.right();
}

// msize

template <typename T, typename L>
std::size_t msize(const adt::either<L, T> &xs) {
  return !mempty(xs);
}
}

#define FUN_EITHER_HPP_DONE
#endif // #ifdef FUN_EITHER_HPP
#ifndef FUN_EITHER_HPP_DONE
#error "Cyclic include dependency"
#endif
