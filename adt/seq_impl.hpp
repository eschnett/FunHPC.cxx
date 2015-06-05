#ifndef ADT_SEQ_IMPL_HPP
#define ADT_SEQ_IMPL_HPP

#include "seq_decl.hpp"

#include <fun/vector.hpp>

#include <cereal/access.hpp>
#include <cereal/types/utility.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace adt {

template <typename A, typename B, typename T> struct seq {
  // seq<A,B,T> = (A<T>, B<T>)

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

  std::pair<left_constructor<T>, right_constructor<T>> data;

  template <typename Archive> void serialize(Archive &ar) { ar(data); }
};
template <typename A, typename B, typename T>
void swap(seq<A, B, T> &x, seq<A, B, T> &y) {
  swap(x.data, y.data);
}
}

#define ADT_SEQ_IMPL_HPP_DONE
#endif // #ifdef ADT_SEQ_IMPL_HPP
#ifndef ADT_SEQ_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
