#ifndef FUN_SEQ_IMPL_HPP
#define FUN_SEQ_IMPL_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/apply.hpp>
#include <cxx/invoke.hpp>
#include <cxx/tuple.hpp>

#include <adt/seq_impl.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace fun {

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_seq<C>::value> *, typename R, typename CR>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  std::size_t sz = inds.size();
  std::size_t lsz = std::min(sz, fun_traits<typename C::left_dummy>::max_size);
  adt::irange_t linds(inds.imin(), inds.imin() + lsz * inds.istep(),
                      inds.istep());
  adt::irange_t rinds(inds.imin() + lsz * inds.istep(), inds.imax(),
                      inds.istep());
  return CR{{iotaMap<typename C::left_dummy>(f, linds, args...),
             iotaMap<typename C::right_dummy>(f, rinds, args...)}};
}

// fmap

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CT, typename R, typename CR>
CR fmap(F &&f, const adt::seq<A, B, T> &xs, Args &&... args) {
  return CR{
      {fmap(f, xs.data.first, args...), fmap(f, xs.data.second, args...)}};
}

template <typename F, typename A, typename B, typename T, typename T2,
          typename... Args, typename CT, typename R, typename CR>
CR fmap2(F &&f, const adt::seq<A, B, T> &xs, const adt::seq<A, B, T2> &ys,
         Args &&... args) {
  return CR{{fmap2(f, xs.data.first, ys.data.first, args...),
             fmap2(f, xs.data.second, ys.data.second, args...)}};
}

template <typename F, typename A, typename B, typename T, typename T2,
          typename T3, typename... Args, typename CT, typename R, typename CR>
CR fmap3(F &&f, const adt::seq<A, B, T> &xs, const adt::seq<A, B, T2> &ys,
         const adt::seq<A, B, T3> &zs, Args &&... args) {
  return CR{
      {fmap3(f, xs.data.first, ys.data.first, zs.data.first, args...),
       fmap3(f, xs.data.second, ys.data.second, zs.data.second, args...)}};
}

// head, last

template <typename A, typename B, typename T>
decltype(auto) head(const adt::seq<A, B, T> &xs) {
  if (!mempty(xs.data.first))
    return head(xs.data.first);
  return head(xs.data.second);
}

template <typename A, typename B, typename T>
decltype(auto) last(const adt::seq<A, B, T> &xs) {
  if (!mempty(xs.data.second))
    return last(xs.data.second);
  return last(xs.data.first);
}

// foldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args, typename R>
R foldMap(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(op, foldMap(f, op, z, xs.data.first, args...),
                     foldMap(f, op, z, xs.data.second, args...));
}

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename T2, typename... Args, typename R>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs,
           const adt::seq<A, B, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::invoke(
      op, foldMap2(f, op, z, xs.data.first, ys.data.first, args...),
      foldMap2(f, op, z, xs.data.second, ys.data.second, args...));
}

// munit

template <typename C, typename T, std::enable_if_t<detail::is_seq<C>::value> *,
          typename CT>
CT munit(T &&x) {
  if (fun_traits<typename C::left_dummy>::max_size >= 1)
    return CT{{munit<typename C::left_dummy>(std::forward<T>(x)),
               mzero<typename C::right_dummy, std::decay_t<T>>()}};
  return CT{{mzero<typename C::left_dummy, std::decay_t<T>>(),
             munit<typename C::right_dummy>(std::forward<T>(x))}};
}

#if 0

// mjoin

template <typename A, typename B, typename T, typename CT = adt::seq<A, B, T>>
CT mjoin(const adt::seq<A, B, adt::seq<A, B, T>> &xss);

// mbind

template <typename F, typename A, typename B, typename T, typename... Args,
          typename CR>
CR mbind(F &&f, const adt::seq<A, B, T> &xs, Args &&... args) {
  static_assert(detail::is_seq<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

#endif

// mextract

template <typename A, typename B, typename T>
decltype(auto) mextract(const adt::seq<A, B, T> &xs) {
  return head(xs);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename A, typename B,
          typename T, typename... Args, typename CT, typename R, typename CR>
CR mfoldMap(F &&f, Op &&op, Z &&z, const adt::seq<A, B, T> &xs,
            Args &&... args) {
  return munit<typename fun_traits<CT>::dummy>(
      foldMap(std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
              std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R, std::enable_if_t<detail::is_seq<C>::value> *,
          typename CR>
CR mzero() {
  return CR{{mzero<typename C::left_dummy, R>(),
             mzero<typename C::right_dummy, R>()}};
}

#if 0

// mplus

template <typename A, typename B, typename T, typename... Ts, typename CT>
CT mplus(const adt::seq<A, B, T> &xs, const adt::seq<A, B, Ts> &... yss) {
  return CT{{xs.data.first,
             mplus(xs.data.second, mplus(munit<B>(mextract(yss.data.first)),
                                         yss.data.second)...)}};
}

// msome

template <typename C, typename T, typename... Ts,
          std::enable_if_t<detail::is_seq<C>::value> *, typename CT>
CT msome(T &&x, Ts &&... ys) {
  if (fun_traits<typename CT::left_dummy>::max_size == 0)
    return CT{{mzero<typename C::left_dummy, std::decay_t<T>>(),
               msome<typename C::right_dummy>(std::forward<T>(x),
                                              std::forward<Ts>(ys)...)}};
  std::size_t sz = 1 + sizeof...(Ts);
  if (sz <= fun_traits<typename CT::left_dummy>::max_size)
    return CT{{msome<typename C::left_dummy>(std::forward<T>(x),
                                             std::forward<Ts>(ys)...),
               mzero<typename C::right_dummy, std::decay_t<T>>()}};
  std::size_t lsz = fun_traits<typename CT::left_dummy>::max_size;
  std::size_t rsz = sz - lsz;
  return CT{{cxx::apply(msome<typename C::left_dummy>,
                        cxx::tuple_section<0, lsz>(std::forward_as_tuple(
                            std::forward<T>(x), std::forward<Ts>(ys)...))),
             cxx::apply(msome<typename C::right_dummy>,
                        cxx::tuple_section<lsz, rsz>(
                            std::forward_as_tuple(std::forward<Ts>(ys)...)))}};
}

#endif

// mempty

template <typename A, typename B, typename T>
bool mempty(const adt::seq<A, B, T> &xs) {
  return mempty(xs.data.first) && mempty(xs.data.second);
}

// msize

template <typename A, typename B, typename T>
std::size_t msize(const adt::seq<A, B, T> &xs) {
  return msize(xs.data.first) + msize(xs.data.second);
}
}

#define FUN_SEQ_IMPL_HPP_DONE
#endif // #ifdef FUN_SEQ_IMPL_HPP
#ifndef FUN_SEQ_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
