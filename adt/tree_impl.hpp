#ifndef ADT_TREE_IMPL_HPP
#define ADT_TREE_IMPL_HPP

#include "tree_decl.hpp"

#include <adt/dummy.hpp>
#include <fun/either.hpp>

#include <cereal/access.hpp>

#include <type_traits>

namespace adt {

template <typename A, typename T> struct tree {
  // data Tree a = Leaf (A a) | Branch (A (Tree a))

  static_assert(
      std::is_same<typename fun::fun_traits<A>::value_type, adt::dummy>::value,
      "");

  typedef A array_dummy;
  template <typename U>
  using array_constructor =
      typename fun::fun_traits<A>::template constructor<U>;
  typedef T value_type;

  typedef adt::either<T, array_constructor<tree>> either_t;
  either_t subtrees;

  template <typename Archive> void serialize(Archive &ar) { ar(subtrees); }
};
template <typename A, typename T> void swap(tree<A, T> &x, tree<A, T> &y) {
  swap(x.subtrees, y.subtrees);
}
}

#define ADT_TREE_IMPL_HPP_DONE
#endif // #ifdef ADT_TREE_IMPL_HPP
#ifndef ADT_TREE_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
