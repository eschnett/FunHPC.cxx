#ifndef CXX_MONAD_SHARED_PTR_HH
#define CXX_MONAD_SHARED_PTR_HH

#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <memory>
#include <type_traits>

namespace cxx {

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type
unit(T1 &&x) {
  return std::make_shared<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type
make(As &&... as) {
  return std::make_shared<T>(std::forward<As>(as)...);
}

template <typename T, typename F, typename CT = std::shared_ptr<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
C<R> bind(const std::shared_ptr<T> &xs, const F &f) {
  if (!xs)
    return C<R>();
  return cxx::invoke(f, *xs);
}

template <typename T, typename CCT = std::shared_ptr<std::shared_ptr<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::element_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const std::shared_ptr<std::shared_ptr<T> > &xss) {
  if (!xss)
    return C<T>();
  return *xss;
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type zero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = std::shared_ptr<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
plus(const std::shared_ptr<T> &xs, const As &... as) {
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    if (*xss[i])
      return *xss[i];
  return zero<C, T>();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type
some(T &&x) {
  return unit<C>(std::forward<T>(x));
}
}

#endif // #ifndef CXX_MONAD_SHARED_PTR_HH
