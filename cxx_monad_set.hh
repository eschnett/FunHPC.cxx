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
          typename T = std::decay_t<T1> >
std::enable_if_t<cxx::is_set<C<T> >::value, C<T> > munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_set<C<T> >::value, C<T> > mmake(As &&... as) {
  C<T> rs;
  rs.emplace(std::forward<As>(as)...);
  return rs;
}

template <typename T, typename F, typename... As, typename CT = std::set<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
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
std::enable_if_t<cxx::is_set<C<T> >::value, C<T> > mzero() {
  return C<T>();
}

template <typename T, typename... Ts>
auto mplus(const std::set<T> &xs, const std::set<Ts> &... xss) {
  static_assert(cxx::all<std::is_same<Ts, T>::value...>::value, "");
  std::set<T> rs(xs);
  for (auto pxs : { &xss... })
    rs.insert(pxs->begin(), pxs->end());
  return rs;
}

template <template <typename> class C, typename T, typename... Ts,
          std::enable_if_t<cxx::is_set<C<T> >::value> * = nullptr>
auto msome(const T &x, const Ts &... xs) {
  static_assert(cxx::all<std::is_same<Ts, T>::value...>::value, "");
  return C<T>{ x, xs... };
}
}

#define CXX_MONAD_SET_HH_DONE
#else
#ifndef CXX_MONAD_SET_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_MONAD_SET_HH
