#ifndef ADT_NESTED_IMPL_HPP
#define ADT_NESTED_IMPL_HPP

#include "nested_decl.hpp"

#include <adt/dummy.hpp>

#include <cereal/access.hpp>

#include <type_traits>

namespace adt {

template <typename P, typename A, typename T> struct nested {
  // nested<P,A,T> = P<A<T>>

  static_assert(
      std::is_same<typename fun::fun_traits<P>::value_type, adt::dummy>::value,
      "");
  static_assert(
      std::is_same<typename fun::fun_traits<A>::value_type, adt::dummy>::value,
      "");

  typedef P pointer_dummy;
  template <typename U>
  using pointer_constructor =
      typename fun::fun_traits<P>::template constructor<U>;
  typedef A array_dummy;
  template <typename U>
  using array_constructor =
      typename fun::fun_traits<A>::template constructor<U>;
  typedef T value_type;

  pointer_constructor<array_constructor<T>> data;

  template <typename Archive> void serialize(Archive &ar) { ar(data); }
};
template <typename P, typename A, typename T>
void swap(nested<P, A, T> &x, nested<P, A, T> &y) {
  swap(x.data, y.data);
}
}

#define ADT_NESTED_IMPL_HPP_DONE
#endif // #ifdef ADT_NESTED_IMPL_HPP
#ifndef ADT_NESTED_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
