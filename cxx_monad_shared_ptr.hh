#ifndef CXX_MONAD_SHARED_PTR_HH
#define CXX_MONAD_SHARED_PTR_HH

#include "cxx_utils.hh"

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>

namespace cxx {
namespace monad {

namespace detail {
template <typename T> struct is_std_shared_ptr : std::false_type {};
template <typename T>
struct is_std_shared_ptr<std::shared_ptr<T> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_std_shared_ptr<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return std::make_shared<typename std::decay<T>::type>(std::forward<T>(x));
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_std_shared_ptr<M<T> >::value, M<T> >::type
make(As &&... as) {
  return std::make_shared<T>(std::forward<As>(as)...);
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_shared_ptr<M<T> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  if (!x)
    return std::shared_ptr<R>();
  return cxx::invoke(f, *x);
}

#if 0
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_shared_ptr<M<T> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, R>::value)),
    M<R> >::type
fmap(const F &f, const M<T> &x) {
  if (!x)
    return std::shared_ptr<R>();
  return std::make_shared<R>(cxx::invoke(f, *x));
}
#endif

namespace detail {
template <typename T> struct unwrap_std_shared_ptr {
  typedef T type;
  bool is_nonnull(const T &x) const { return true; }
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_std_shared_ptr<std::shared_ptr<T> > {
  typedef T type;
  bool is_nonnull(const std::shared_ptr<T> &x) const { return bool(x); }
  const T &operator()(const std::shared_ptr<T> &x) const { return *x; }
};
}

template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((detail::is_std_shared_ptr<M<R> >::value) &&
     (std::is_same<typename invoke_of<F, typename detail::unwrap_std_shared_ptr<
                                             As>::type...>::type,
                   R>::value)),
    M<R> >::type
fmap(const F &f, const As &... as) {
  std::array<bool, sizeof...(As)> is_nonnulls = {
    { detail::unwrap_std_shared_ptr<As>().is_nonnull(as)... }
  };
  bool is_nonnull = *std::min_element(is_nonnulls.begin(), is_nonnulls.end());
  if (!is_nonnull)
    return std::make_shared<R>();
  return std::make_shared<R>(
      cxx::invoke(f, detail::unwrap_std_shared_ptr<As>()(as)...));
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_shared_ptr<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  if (!x)
    return std::shared_ptr<T>();
  return *x;
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_shared_ptr<M<T> >::value, M<T> >::type
zero() {
  return std::shared_ptr<T>();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_shared_ptr<M<T> >::value, M<T> >::type
plus(const M<T> &x, const M<T> &y) {
  return x ? x : y;
}
}
}

#endif // #ifndef CXX_MONAD_SHARED_PTR_HH
