#ifndef CXX_MONAD_LIST_HH
#define CXX_MONAD_LIST_HH

#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <list>
#include <type_traits>

namespace cxx {

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_list<C<T> >::value, C<T> >::type munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_list<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  C<T> rs;
  rs.emplace_back(std::forward<As>(as)...);
  return rs;
}

template <typename T, typename F, typename... As, typename CT = std::list<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const std::list<T> &xs, const F &f, const As &... as) {
  C<R> rs;
  for (const auto &x : xs)
    rs.splice(rs.end(), cxx::invoke(f, x, as...));
  return rs;
}

template <typename T, typename CCT = std::list<std::list<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const std::list<std::list<T> > &xss) {
  C<T> rs;
  for (const auto &xs : xss)
    rs.splice(rs.end(), xs);
  return rs;
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_list<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = std::list<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
mplus(const std::list<T> &xs, const As &... as) {
  C<T> rs(xs);
  std::array<const C<T> *, sizeof...(As)> xss{ &as... };
  for (size_t i = 0; i < xss.size(); ++i)
    rs.insert(rs.end(), *xss[i]);
  return rs;
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<((cxx::is_list<C<T> >::value) &&
                         (cxx::all<std::is_same<As, T>::value...>::value)),
                        C<T> >::type
msome(const T &x, const As &... as) {
  C<T> rs;
  rs.push_back(x);
  std::array<const T *, sizeof...(As)> xs{ &as... };
  for (size_t i = 0; i < xs.size(); ++i)
    rs.push_back(*xs[i]);
  return rs;
}
}

#endif // #ifndef CXX_MONAD_LIST_HH
