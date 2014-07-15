#ifndef CXX_MONAD_VECTOR_HH
#define CXX_MONAD_VECTOR_HH

#include "cxx_utils.hh"

#include <algorithm>
#include <array>
#include <vector>
#include <type_traits>

namespace cxx {
namespace monad {

namespace detail {
template <typename T> struct is_std_vector : std::false_type {};
template <typename T, typename Allocator>
struct is_std_vector<std::vector<T, Allocator> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_std_vector<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return { std::forward<T>(x) };
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
make(As &&... as) {
  M<T> r;
  r.emplace_back(std::forward<As>(as)...);
  return r;
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_vector<M<T> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  M<R> r;
  for (const auto &e : x) {
    M<R> y = cxx::invoke(f, e);
    std::move(y.begin(), y.end(), std::inserter(r, r.end()));
  }
  return r;
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
  std::array<std::size_t, sizeof...(As)> sizes = {
    { detail::unwrap_std_vector<As>().size(as)... }
  };
  std::size_t s = *std::max_element(sizes.begin(), sizes.end());
  M<R> r;
  r.reserve(s);
  for (std::size_t i = 0; i < s; ++i)
    r.push_back(cxx::invoke(f, detail::unwrap_std_vector<As>()(as, i)...));
  return r;
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  M<T> r;
  for (const auto &e : x) {
    r.insert(r.end(), e.begin(), e.end());
  }
  return r;
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
zero() {
  return M<T>();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
some() {
  return zero<M, T>();
}
template <template <typename> class M, typename T, typename... As>
typename std::enable_if<
    ((detail::is_std_vector<M<typename std::decay<T>::type> >::value) &&
     (cxx::all<std::is_same<typename std::decay<As>::type,
                            typename std::decay<T>::type>::value...>::value)),
    M<typename std::decay<T>::type> >::type
some(T &&x, As &&... as) {
  return { std::forward<T>(x), std::forward<As>(as)... };
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
plus() {
  return zero<M, T>();
}
template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_vector<M<T> >::value, M<T> >::type
plus(const M<T> &x) {
  return x;
}
template <template <typename> class M, typename T, typename... As>
typename std::enable_if<((detail::is_std_vector<M<T> >::value) &&
                         (cxx::all<std::is_same<As, T>::value...>::value)),
                        M<T> >::type
plus(const M<T> &x, const M<As> &... as) {
  M<T> r(x);
  std::array<const M<T> *, sizeof...(As)> xs = { { &as... } };
  for (auto x : xs)
    r.insert(r.end(), x->begin(), x->end());
  return r;
}
}
}

#endif // #ifndef CXX_MONAD_VECTOR_HH
