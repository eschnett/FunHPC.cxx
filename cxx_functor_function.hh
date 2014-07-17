#ifndef CXX_FUNCTOR_FUNCTION_HH
#define CXX_FUNCTOR_FUNCTION_HH

#include "cxx_invoke.hh"

#include <functional>
#include <type_traits>

namespace cxx {
namespace functor {

namespace detail {
template <typename T> struct is_std_function : std::false_type {};
template <typename T, typename R>
struct is_std_function<std::function<R(T)> > : std::true_type {};
}

// fmap: (a -> b) -> m a -> m b
// fmap: (a -> b) -> (r -> a) -> r -> b
// fmap f g = \x -> f (g x)
// TODO: allow more arguments?
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_function<M<R> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, R>::value)),
    M<R> >::type
fmap(const F &f, const M<T> &g) {
  return M<R>([f, g](const typename M<T>::argument_type &x) {
    return cxx::invoke(f, cxx::invoke(g, x));
  });
}
}
}

#endif // #ifndef CXX_FUNCTOR_FUNCTION_HH
