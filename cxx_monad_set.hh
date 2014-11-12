#ifndef CXX_MONAD_SET_HH
#define CXX_MONAD_SET_HH

#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <set>
#include <type_traits>

namespace cxx {

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_set<C<T> >::value, C<T> >::type munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_set<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  C<T> rs;
  rs.emplace(std::forward<As>(as)...);
  return rs;
}

template <typename T, typename F, typename... As, typename CT = std::set<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const std::set<T> &xs, const F &f, const As &... as) {
  C<R> rs;
  for (const auto &x : xs) {
    C<R> y = cxx::invoke(f, x, as...);
    rs.insert(y.begin(), y.end());
  }
  return rs;
}

template <typename T, typename CCT = std::set<std::set<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const std::set<std::set<T> > &xss) {
  C<T> rs;
  for (const auto &xs : xss)
    rs.insert(xs.begin(), xs.end());
  return rs;
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_set<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = std::set<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
mplus(const std::set<T> &xs, const As &... as) {
  C<T> rs(xs);
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    rs.insert(xss[i]->begin(), xss[i]->end());
  return rs;
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<((cxx::is_set<C<T> >::value) &&
                         (cxx::all<std::is_same<As, T>::value...>::value)),
                        C<T> >::type
msome(const T &x, const As &... as) {
  C<T> rs;
  rs.insert(x);
  std::array<const T *, sizeof...(As)> xs{ &as... };
  for (size_t i = 0; i < xs.size(); ++i)
    rs.insert(*xs[i]);
  return rs;
}
}

#endif // #ifndef CXX_MONAD_SET_HH
