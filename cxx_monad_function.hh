#ifndef CXX_MONAD_FUNCTION_HH
#define CXX_MONAD_FUNCTION_HH

#include "cxx_functor_function.hh"

#include "cxx_invoke.hh"

#include <functional>
#include <type_traits>

namespace cxx {
namespace monad {

using cxx::functor::fmap;

// function: (->) r = r -> *

namespace detail {
template <typename T> struct is_std_function : std::false_type {};
template <typename T, typename R>
struct is_std_function<std::function<R(T)> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_std_function<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return M<typename std::decay<T>::type>([x](
      const typename M<typename std::decay<T>::type>::argument_type &) {
    return x;
  });
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_std_function<M<T> >::value, M<T> >::type
make(const T &x) {
  return M<T>([x](const typename M<T>::argument_type &) { return x; });
}

// bind: m a -> (a -> m b) -> m b
// bind: (r -> a) -> (a -> (r -> b)) -> r -> b
// bind: f g = \x -> (g (f x)) x
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<(detail::is_std_function<M<T> >::value &&std::is_same<
                            typename invoke_of<F, T>::type, M<R> >::value),
                        M<R> >::type
bind(const M<T> &f, const F &g) {
  return M<R>([f, g](const T &x) {
    return cxx::invoke(cxx::invoke(g, cxx::invoke(f, x)), x);
  });
}

// join: m (m a) -> m a
// join: (r -> (r -> a)) -> r -> a
// join f = \x -> (f x) x
template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_function<M<T> >::value, M<T> >::type
join(const M<M<T> > &f) {
  return M<T>([f](const T &x) { return cxx::invoke(cxx::invoke(f, x), x); });
}
}
}

#endif // #ifndef CXX_MONAD_FUNCTION_HH
