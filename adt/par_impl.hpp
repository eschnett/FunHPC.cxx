#ifndef ADT_PAR_IMPL_HPP
#define ADT_PAR_IMPL_HPP

#include "par_decl.hpp"

#include <adt/either.hpp>
#include <fun/vector.hpp>

#include <cereal/access.hpp>

#include <cstddef>
#include <type_traits>

namespace adt {

template <typename A, typename B, typename T> struct par {
  // par<A,B,T> = either A<T> B<T>

  static_assert(
      std::is_same<typename fun::fun_traits<A>::value_type, adt::dummy>::value,
      "");
  static_assert(
      std::is_same<typename fun::fun_traits<B>::value_type, adt::dummy>::value,
      "");

  typedef A left_dummy;
  typedef B right_dummy;
  template <typename U>
  using left_constructor = typename fun::fun_traits<A>::template constructor<U>;
  template <typename U>
  using right_constructor =
      typename fun::fun_traits<B>::template constructor<U>;

  typedef adt::either<left_constructor<T>, right_constructor<T>> either_t;
  either_t data;

  template <typename Archive> void serialize(Archive &ar) { ar(data); }
};
template <typename A, typename B, typename T>
void swap(par<A, B, T> &x, par<A, B, T> &y) {
  swap(x.data, y.data);
}
} // namespace adt

#define ADT_PAR_IMPL_HPP_DONE
#endif // #ifdef ADT_PAR_IMPL_HPP
#ifndef ADT_PAR_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
