#ifndef FUN_PAR_IMPL_HPP
#define FUN_PAR_IMPL_HPP

#include "par_decl.hpp"

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>

#include <adt/par_impl.hpp>
#include <fun/fun_impl.hpp>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <type_traits>

namespace fun {

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_par<C>::value> *, typename R, typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  std::size_t sz = inds.size();
  if (sz >= fun_traits<typename CR::left_dummy>::min_size() &&
      sz <= fun_traits<typename CR::left_dummy>::max_size())
    return CR{CR::either_t::make_left(iotaMap<typename CR::left_dummy>(
        std::forward<F>(f), inds, std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(iotaMap<typename CR::right_dummy>(
      std::forward<F>(f), inds, std::forward<Args>(args)...))};
}

// fmap

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CT, typename R, typename CR>
CR fmap(F &&f, const adt::par<A, B, T> &xs, Args &&... args) {
  bool s = xs.data.right();
  if (!s)
    return CR{CR::either_t::make_left(fmap(
        std::forward<F>(f), xs.data.get_left(), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(fmap(
      std::forward<F>(f), xs.data.get_right(), std::forward<Args>(args)...))};
}

template <typename F, typename A, typename B, typename T, typename T2,
          typename... Args, typename CT, typename R, typename CR>
CR fmap2(F &&f, const adt::par<A, B, T> &xs, const adt::par<A, B, T2> &ys,
         Args &&... args) {
  bool s = xs.data.right();
  cxx_assert(ys.data.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        fmap2(std::forward<F>(f), xs.data.get_left(), ys.data.get_left(),
              std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(
      fmap2(std::forward<F>(f), xs.data.get_right(), ys.data.get_right(),
            std::forward<Args>(args)...))};
}

template <typename F, typename A, typename B, typename T, typename T2,
          typename T3, typename... Args, typename CT, typename R, typename CR>
CR fmap3(F &&f, const adt::par<A, B, T> &xs, const adt::par<A, B, T2> &ys,
         const adt::par<A, B, T3> &zs, Args &&... args) {
  bool s = xs.data.right();
  cxx_assert(ys.data.right() == s);
  cxx_assert(zs.data.right() == s);
  if (!s)
    return CR{CR::either_t::make_left(
        fmap3(std::forward<F>(f), xs.data.get_left(), ys.data.get_left(),
              zs.data.get_left(), std::forward<Args>(args)...))};
  return CR{CR::either_t::make_right(
      fmap3(std::forward<F>(f), xs.data.get_right(), ys.data.get_right(),
            zs.data.get_right(), std::forward<Args>(args)...))};
}

// head, last

template <typename A, typename B, typename T>
decltype(auto) head(const adt::par<A, B, T> &xs) {
  if (xs.data.left())
    return head(xs.data.get_left());
  return head(xs.data.get_right());
}

template <typename A, typename B, typename T>
decltype(auto) last(const adt::par<A, B, T> &xs) {
  if (xs.data.left())
    return last(xs.data.get_left());
  return last(xs.data.get_right());
}

// foldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::par<A, B, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.data.right();
  if (!s)
    return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                   xs.data.get_left(), std::forward<Args>(args)...);
  return foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                 xs.data.get_right(), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename T2, typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::par<A, B, T> &xs,
           const adt::par<A, B, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.data.right();
  cxx_assert(ys.data.right() == s);
  if (!s)
    return foldMap2(std::forward<F>(f), std::forward<Op>(op),
                    std::forward<Z>(z), xs.data.get_left(), ys.data.get_left(),
                    std::forward<Args>(args)...);
  return foldMap2(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z),
                  xs.data.get_right(), ys.data.get_right(),
                  std::forward<Args>(args)...);
}

// dump

template <typename A, typename B, typename T>
ostreamer dump(const adt::par<A, B, T> &xs) {
  bool s = xs.data.right();
  std::ostringstream os;
  os << "par{";
  if (!s)
    os << "left{" << xs.data.get_left() << "}";
  else
    os << "right{" << xs.data.get_right() << "}";
  os << "}";
  return ostreamer(os.str());
}

// munit

template <typename C, typename T, std::enable_if_t<detail::is_par<C>::value> *,
          typename CT>
CT munit(T &&x) {
  if (fun_traits<typename CT::left_dummy>::min_size() <= 1 &&
      fun_traits<typename CT::left_dummy>::max_size() >= 1)
    return CT{CT::either_t::make_left(
        munit<typename C::left_dummy>(std::forward<T>(x)))};
  return CT{CT::either_t::make_right(
      munit<typename C::right_dummy>(std::forward<T>(x)))};
}

#if 0

// mjoin

template <typename A, typename B, typename T, typename CT = adt::par<A, B, T>>
CT mjoin(const adt::par<A, B, adt::par<A, B, T>> &xss);

// mbind

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CR>
CR mbind(F &&f, const adt::par<A, B, T> &xs, Args &&... args) {
  static_assert(detail::is_par<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

#endif

// mextract

template <typename A, typename B, typename T>
decltype(auto) mextract(const adt::par<A, B, T> &xs) {
  return head(xs);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args, typename CT, typename R, typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::par<A, B, T> &xs,
            Args &&... args) {
  return munit<typename fun_traits<CT>::dummy>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R, std::enable_if_t<detail::is_par<C>::value> *,
          typename CR>
CR mzero() {
  if (fun_traits<typename CR::left_dummy>::min_size() == 0)
    return CR{CR::either_t::make_left(mzero<typename C::left_dummy, R>())};
  return CR{CR::either_t::make_right(mzero<typename C::right_dummy, R>())};
}

#if 0

// mplus

template <typename A, typename B, typename T, typename... Ts,
          typename CT = adt::par<A, B, T>>
CT mplus(const adt::par<A, B, T> &xs, const adt::par<A, B, Ts> &... yss);

#endif

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_par<C>::value> *, typename CT>
CT msome(T &&x, Ts &&... ys) {
  std::size_t sz = 1 + sizeof...(Ts);
  if (sz >= fun_traits<typename CT::left_dummy>::min_size() &&
      sz <= fun_traits<typename CT::left_dummy>::max_size())
    return CT{CT::either_t::make_left(msome<typename C::left_dummy>(
        std::forward<T>(x), std::forward<Ts>(ys)...))};
  return CT{CT::either_t::make_right(msome<typename C::right_dummy>(
      std::forward<T>(x), std::forward<Ts>(ys)...))};
}

// mempty

template <typename A, typename B, typename T>
bool mempty(const adt::par<A, B, T> &xs) {
  bool s = xs.data.right();
  if (!s)
    return mempty(xs.data.get_left());
  return mempty(xs.data.get_right());
}

// msize

template <typename A, typename B, typename T>
std::size_t msize(const adt::par<A, B, T> &xs) {
  bool s = xs.data.right();
  if (!s)
    return msize(xs.data.get_left());
  return msize(xs.data.get_right());
}
}

#define FUN_PAR_IMPL_HPP_DONE
#endif // #ifdef FUN_PAR_IMPL_HPP
#ifndef FUN_PAR_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
