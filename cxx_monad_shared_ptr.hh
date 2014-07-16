#ifndef CXX_MONAD_SHARED_PTR_HH
#define CXX_MONAD_SHARED_PTR_HH

#include "cxx_functor_shared_ptr.hh"

#include "cxx_utils.hh"

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>

namespace cxx {
namespace monad {

using cxx::functor::fmap;

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
