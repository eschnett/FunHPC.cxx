// -*-C++-*-
#ifndef FUN_SHARED_FUTURE_HPP
#define FUN_SHARED_FUTURE_HPP

#include <cxx/invoke.hpp>
#include <qthread/future.hpp>

#include <cassert>
#include <algorithm>
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

// iota

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<F, std::size_t, Args...>,
          std::enable_if_t<detail::is_shared_future<C<R>>::value> * = nullptr>
auto iota(F &&f, std::size_t s, Args &&... args) {
  assert(s == 1);
  return qthread::make_ready_future(
             cxx::invoke(std::forward<F>(f), std::size_t(0),
                         std::forward<Args>(args)...)).share();
}

// fmap

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  bool s = xs.valid();
  assert(s);
  return qthread::make_ready_future(cxx::invoke(std::forward<F>(f), xs.get(),
                                                std::forward<Args>(args)...))
      .share();
}

template <typename F, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
auto fmap(F &&f, qthread::shared_future<T> &&xs, Args &&... args) {
  bool s = xs.valid();
  assert(s);
  return qthread::make_ready_future(
             cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                         std::forward<Args>(args)...)).share();
}

template <typename F, typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
auto fmap2(F &&f, const qthread::shared_future<T> &xs,
           const qthread::shared_future<T2> &ys, Args &&... args) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  assert(s);
  return qthread::make_ready_future(
             cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                         std::forward<Args>(args)...)).share();
}

// foldMap

template <typename F, typename Op, typename R, typename T, typename... Args>
R foldMap(F &&f, Op &&op, const R &z, const qthread::shared_future<T> &xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  assert(s);
  return cxx::invoke(
      std::forward<Op>(op), z,
      cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...));
}

template <typename F, typename Op, typename R, typename T, typename... Args>
R foldMap(F &&f, Op &&op, const R &z, qthread::shared_future<T> &&xs,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  assert(s);
  return cxx::invoke(std::forward<Op>(op), z,
                     cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                                 std::forward<Args>(args)...));
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... Args>
R foldMap2(F &&f, Op &&op, const R &z, const qthread::shared_future<T> &xs,
           const qthread::shared_future<T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  assert(s);
  return cxx::invoke(std::forward<Op>(op), z,
                     cxx::invoke(std::forward<F>(f), xs.get(), ys.get(),
                                 std::forward<Args>(args)...));
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          std::enable_if_t<detail::is_shared_future<C<R>>::value> * = nullptr>
auto munit(T &&x) {
  return qthread::make_ready_future(std::forward<T>(x)).share();
}

// mbind

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, const qthread::shared_future<T> &xs, Args &&... args) {
  static_assert(detail::is_shared_future<CR>::value, "");
  assert(xs.valid());
  return cxx::invoke(std::forward<F>(f), xs.get(), std::forward<Args>(args)...);
}

template <typename F, typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
auto mbind(F &&f, qthread::shared_future<T> &&xs, Args &&... args) {
  static_assert(detail::is_shared_future<CR>::value, "");
  assert(xs.valid());
  return cxx::invoke(std::forward<F>(f), std::move(xs.get()),
                     std::forward<Args>(args)...);
}

// mjoin

template <typename T>
auto mjoin(const qthread::shared_future<qthread::shared_future<T>> &xss) {
  assert(xss.valid());
  return qthread::async([xss]() { return xss.get().get(); }).share();
}

template <typename T>
auto mjoin(qthread::shared_future<qthread::shared_future<T>> &&xss) {
  assert(xss.valid());
  return qthread::async([xss = std::move(xss)]() { return xss.get().get(); })
      .share();
}

// mextract

template <typename T>
decltype(auto) mextract(const qthread::shared_future<T> &xs) {
  assert(xs.valid());
  return xs.get();
}

template <typename T> decltype(auto) mextract(qthread::shared_future<T> &&xs) {
  assert(xs.valid());
  return std::move(xs.get());
}
}

#define FUN_SHARED_FUTURE_HPP_DONE
#endif // #ifdef FUN_SHARED_FUTURE_HPP
#ifndef FUN_SHARED_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
