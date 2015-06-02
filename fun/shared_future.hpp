#ifndef FUN_SHARED_FUTURE_HPP
#define FUN_SHARED_FUTURE_HPP

#include <qthread/future.hpp>

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>

namespace fun {

// is_shared_future

namespace detail {
template <typename> struct is_shared_future : std::false_type {};
template <typename T>
struct is_shared_future<qthread::shared_future<T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T> struct fun_traits<qthread::shared_future<T>> {
  template <typename U>
  using constructor = qthread::shared_future<std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename R = std::decay_t<cxx::invoke_of_t<
              std::decay_t<F>, std::ptrdiff_t, std::decay_t<Args>...>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  assert(inds.size() <= 1);
  if (__builtin_expect(inds.empty(), false))
    return qthread::shared_future<R>();
  return qthread::async(std::forward<F>(f), inds[0],
                        std::forward<Args>(args)...).share();
}

// fmap

template <typename F, typename T, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = std::decay_t<std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  bool s = xs.valid();
  if (!s)
    return qthread::shared_future<R>();
  return xs.then([ f = std::forward<F>(f), args... ](
                     const qthread::shared_future<T> &xs) {
    return cxx::invoke(std::move(f), xs.get(), std::move(args)...);
  }).share();
}

template <typename F, typename T, typename T2, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = std::decay_t<std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, T2, std::decay_t<Args>...>>>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const qthread::shared_future<T> &xs,
         const qthread::shared_future<T2> &ys, Args &&... args) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (!s)
    return qthread::shared_future<R>();
  return xs.then([ f = std::forward<F>(f), ys, args... ](
                     const qthread::shared_future<T> &xs) mutable {
    return cxx::invoke(std::move(f), xs.get(), ys.get(), std::move(args)...);
  }).share();
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, T, std::decay_t<Args>...>>>
R foldMap(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
          Args &&... args) {
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<std::decay_t<Op>, R, R>>,
                   R>::value,
      "");
  bool s = xs.valid();
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename Op, typename Z, typename T, typename T2,
          typename... Args, typename R = std::decay_t<cxx::invoke_of_t<
                                std::decay_t<F>, T, T2, std::decay_t<Args>...>>>
R foldMap2(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
           const qthread::shared_future<T2> &ys, Args &&... args) {
  static_assert(
      std::is_same<std::decay_t<cxx::invoke_of_t<std::decay_t<Op>, R, R>>,
                   R>::value,
      "");
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (!s)
    return std::forward<Z>(z);
  return cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                     std::forward<Args>(args)...);
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR munit(T &&x) {
  return qthread::make_ready_future(std::forward<T>(x)).share();
}

// mjoin

template <typename T, typename CT = qthread::shared_future<T>>
CT mjoin(const qthread::shared_future<qthread::shared_future<T>> &xss) {
  assert(xss.valid());
  // return qthread::async([xss]() { return xss.get().get(); }).share();
  return xss.unwrap().share();
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  static_assert(detail::is_shared_future<CR>::value, "");
  assert(xs.valid());
  // return cxx::invoke(std::forward<F>(f), xs.get(),
  // std::forward<Args>(args)...);
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

// mextract

template <typename T> const T &mextract(const qthread::shared_future<T> &xs) {
  assert(xs.valid());
  return xs.get();
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, typename... Args,
          typename C = qthread::shared_future<T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const qthread::shared_future<T> &xs,
            Args &&... args) {
  return qthread::async(
             [](auto &&f, auto &&op, auto &&z, auto &&xs, auto &&... args) {
               return foldMap(std::forward<F>(f), std::forward<Op>(op),
                              std::forward<Z>(z), xs,
                              std::forward<Args>(args)...);
             },
             std::forward<F>(f), std::forward<Op>(op), std::forward<Z>(z), xs,
             std::forward<Args>(args)...).share();
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_shared_future<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mzero() {
  return qthread::shared_future<R>();
}

// mempty

template <typename T>
constexpr bool mempty(const qthread::shared_future<T> &xs) {
  return !xs.valid();
}

// msize

template <typename T>
constexpr std::size_t msize(const qthread::shared_future<T> &xs) {
  return !mempty(xs);
}
}

#define FUN_SHARED_FUTURE_HPP_DONE
#endif // #ifdef FUN_SHARED_FUTURE_HPP
#ifndef FUN_SHARED_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
