#ifndef CXX_MONAD_LIST_HH
#define CXX_MONAD_LIST_HH

#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <list>
#include <type_traits>

namespace cxx {

template <template <typename> class C, typename T1,
          typename T = std::decay_t<T1> >
std::enable_if_t<cxx::is_list<C<T> >::value, C<T> > munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_list<C<T> >::value, C<T> > mmake(As &&... as) {
  C<T> rs;
  rs.emplace_back(std::forward<As>(as)...);
  return rs;
}

template <typename T, typename F, typename... As, typename CT = std::list<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
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
std::enable_if_t<cxx::is_list<C<T> >::value, C<T> > mzero() {
  return C<T>();
}

template <typename T, typename... Ts>
auto mplus(const std::list<T> &xs, const std::list<Ts> &... xss) {
  static_assert(cxx::all<std::is_same<Ts, T>::value...>::value, "");
  std::list<T> rs(xs);
  for (auto pxs : { &xss... })
    rs.insert(rs.end(), pxs->begin(), pxs->end());
  return rs;
}

template <template <typename> class C, typename T, typename... Ts,
          std::enable_if_t<cxx::is_list<C<T> >::value> * = nullptr>
auto msome(const T &x, const Ts &... xs) {
  static_assert(cxx::all<std::is_same<Ts, T>::value...>::value, "");
  return C<T>{ x, xs... };
}
}

#define CXX_MONAD_LIST_HH_DONE
#else
#ifndef CXX_MONAD_LIST_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_MONAD_LIST_HH
