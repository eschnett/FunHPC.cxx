#ifndef CXX_FUNCTOR_VECTOR_HH
#define CXX_FUNCTOR_VECTOR_HH

#include "cxx_utils.hh"

#include <algorithm>
#include <array>
#include <vector>
#include <type_traits>

namespace cxx {
namespace functor {

namespace detail {
template <typename T> struct is_std_vector : std::false_type {};
template <typename T, typename Allocator>
struct is_std_vector<std::vector<T, Allocator> > : std::true_type {};
}

namespace detail {
template <typename T> struct unwrap_std_vector {
  typedef T type;
  std::size_t size(const T &x) const { return 1; }
  const T &operator()(const T &x, std::size_t i) const { return x; }
};
template <typename T, typename Allocator>
struct unwrap_std_vector<std::vector<T, Allocator> > {
  typedef T type;
  std::size_t size(const std::vector<T, Allocator> &x) const {
    return x.size();
  }
  const T &operator()(const std::vector<T, Allocator> &x, std::size_t i) const {
    return x[i];
  }
};
}
template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((detail::is_std_vector<M<R> >::value) &&
     (std::is_same<typename invoke_of<F, typename detail::unwrap_std_vector<
                                             As>::type...>::type,
                   R>::value)),
    M<R> >::type
fmap(const F &f, const As &... as) {
  std::array<std::size_t, sizeof...(As)> sizes{
    { detail::unwrap_std_vector<As>().size(as)... }
  };
  std::size_t s = *std::max_element(sizes.begin(), sizes.end());
  M<R> r;
  r.reserve(s);
  for (std::size_t i = 0; i < s; ++i)
    r.push_back(cxx::invoke(f, detail::unwrap_std_vector<As>()(as, i)...));
  return r;
}
}
}

#endif // #ifndef CXX_FUNCTOR_VECTOR_HH
