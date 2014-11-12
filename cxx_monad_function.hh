#ifndef CXX_MONAD_FUNCTION_HH
#define CXX_MONAD_FUNCTION_HH

#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <functional>
#include <type_traits>

namespace cxx {

// function: (->) r = r -> *

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_function<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return C<T>([x](const typename C<T>::argument_type &) { return x; });
}

template <template <typename> class C, typename T, typename A>
typename std::enable_if<cxx::is_function<C<T> >::value, C<T> >::type
mmake(A &&x) {
  return munit<C>(std::forward<A>(x));
}

// mbind: m a -> (a -> m b) -> m b
// mbind: (r -> a) -> (a -> (r -> b)) -> r -> b
// mbind: f g = \x -> (g (f x)) x
template <typename T, typename A, typename F, typename CT = std::function<T(A)>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const std::function<T(A)> &f, const F &g) {
  return C<R>([f, g](const T &x) {
    return cxx::invoke(cxx::invoke(g, cxx::invoke(f, x)), x);
  });
}

// mjoin: m (m a) -> m a
// mjoin: (r -> (r -> a)) -> r -> a
// mjoin f = \x -> (f x) x
template <typename T, typename A,
          typename CCT = std::function<std::function<T(A)>(A)>,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
typename std::enable_if<std::is_same<C2<T>, C<T> >::value, C<T> >::type
mjoin(const std::function<std::function<T(A)>(A)> &f) {
  return C<T>([f](const T &x) { return cxx::invoke(cxx::invoke(f, x), x); });
}
}

#endif // #ifndef CXX_MONAD_FUNCTION_HH
