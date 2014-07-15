#ifndef CXX_MONAD_SET_HH
#define CXX_MONAD_SET_HH

#include "cxx_invoke.hh"
#include "cxx_utils.hh"

#include <set>
#include <type_traits>

namespace cxx {
namespace monad {

namespace detail {
template <typename T> struct is_std_set : std::false_type {};
template <typename T, typename Compare, typename Allocator>
struct is_std_set<std::set<T, Compare, Allocator> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_std_set<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return { std::forward<T>(x) };
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type
make(As &&... as) {
  M<T> r;
  r.emplace(std::forward<T>(as)...);
  return r;
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_std_set<M<T> >::value) &&
     (std::is_same<typename cxx::invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  M<R> r;
  for (const auto &e : x) {
    M<R> y = cxx::invoke(f, e);
    std::move(y.begin(), y.end(), std::inserter(r, r.end()));
  }
  return r;
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

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  M<T> r;
  for (const auto &e : x) {
    r.insert(r.end(), e.begin(), e.end());
  }
  return r;
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type zero() {
  return M<T>();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type some() {
  return zero<M, T>();
}
template <template <typename> class M, typename T, typename... As>
typename std::enable_if<
    ((detail::is_std_set<M<typename std::decay<T>::type> >::value) &&
     (cxx::all<std::is_same<typename std::decay<As>::type,
                            typename std::decay<T>::type>::value...>::value)),
    M<typename std::decay<T>::type> >::type
some(T &&x, As &&... as) {
  return { std::forward<T>(x), std::forward<As>(as)... };
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type plus() {
  return zero<M, T>();
}
template <template <typename> class M, typename T>
typename std::enable_if<detail::is_std_set<M<T> >::value, M<T> >::type
plus(const M<T> &x) {
  return x;
}
template <template <typename> class M, typename T, typename... As>
typename std::enable_if<((detail::is_std_set<M<T> >::value) &&
                         (cxx::all<std::is_same<As, T>::value...>::value)),
                        M<T> >::type
plus(const M<T> &x, const M<As> &... as) {
  M<T> r(x);
  std::array<const M<T> *, sizeof...(As)> xs{ { &as... } };
  for (auto x : xs)
    r.insert(x->begin(), x->end());
  return r;
}
}
}

#endif // #ifndef CXX_MONAD_SET_HH
