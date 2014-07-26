#ifndef CXX_MONAD_OPERATORS_HH
#define CXX_MONAD_OPERATORS_HH

#include "cxx_kinds.hh"
#include "cxx_invoke.hh"

#include <utility>

// Operator notation for bind
template <typename CT, typename F,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::element_type,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
C<R> operator>>=(const CT &xs, const F &f) {
  return cxx::bind(xs, f);
}
// template <typename CT, typename F,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename T = typename cxx::kinds<CT>::element_type,
//           typename CR = typename cxx::invoke_of<F, T &&>::type,
//           typename R = typename cxx::kinds<CR>::element_type>
// C<R> operator>>=(CT &&xs, const F &f) {
//   return cxx::bind(std::move(xs), f);
// }
template <typename CT, typename R,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::element_type>
C<R> operator>>(const CT &xs, const C<R> &rs) {
  return cxx::bind(xs, [rs](const T &) { return rs; });
}
// template <typename CT, typename R,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename T = typename cxx::kinds<CT>::element_type>
// C<R> operator>>(CT &&xs, const C<R> &rs) {
//   return cxx::bind(std::move(xs), [rs](T &&) { return rs; });
// }

#endif // #ifndef CXX_MONAD_OPERATORS_HH
