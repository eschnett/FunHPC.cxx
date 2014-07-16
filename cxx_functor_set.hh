#ifndef CXX_FUNCTOR_SET_HH
#define CXX_FUNCTOR_SET_HH

#include "cxx_invoke.hh"
#include "cxx_utils.hh"

#include <set>
#include <type_traits>

namespace cxx {
namespace functor {

namespace detail {
template <typename T> struct is_std_set : std::false_type {};
template <typename T, typename Compare, typename Allocator>
struct is_std_set<std::set<T, Compare, Allocator> > : std::true_type {};
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_set<M<R> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, R>::value)),
    M<R> >::type
fmap(const F &f, const M<T> &xs) {
  M<R> r;
  for (const auto &x : xs)
    r.insert(cxx::invoke(f, x));
  return r;
}
}
}

#endif // #ifndef CXX_FUNCTOR_SET_HH
