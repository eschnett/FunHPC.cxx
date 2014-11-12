#error "OUTDATED"

#ifndef CXX_MONAD_OPERATORS_HH
#define CXX_MONAD_OPERATORS_HH

#include "cxx_kinds.hh"
#include "cxx_invoke.hh"

#include <utility>

// TODO: define this separately for each monad

// Operator notation for mbind
template <typename CT, typename F,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::value_type,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> operator>>=(const CT &xs, const F &f) {
  return cxx::mbind(xs, f);
}
// template <typename CT, typename F,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename T = typename cxx::kinds<CT>::value_type,
//           typename CR = typename cxx::invoke_of<F, T &&>::type,
//           typename R = typename cxx::kinds<CR>::value_type>
// C<R> operator>>=(CT &&xs, const F &f) {
//   return cxx::mbind(std::move(xs), f);
// }

namespace cxx {
template <typename CT, typename R,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::value_type>
C<R> mbind0(const CT &xs, const C<R> &rs) {
  return cxx::mbind(xs, [rs](const T &) { return rs; });
}
}

template <typename CT, typename R,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::value_type>
C<R> operator>>(const CT &xs, const C<R> &rs) {
  return cxx::mbind0(xs, rs);
}
// template <typename CT, typename R,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename T = typename cxx::kinds<CT>::value_type>
// C<R> operator>>(CT &&xs, const C<R> &rs) {
//   return cxx::mbind0(std::move(xs), rs);
// }

#endif // #ifndef CXX_MONAD_OPERATORS_HH
