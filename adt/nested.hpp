#ifndef ADT_NESTED_HPP
#define ADT_NESTED_HPP

#include <cereal/access.hpp>

#include <vector>

namespace adt {

// nested<P,A,T> = P<A<T>>
template <template <typename> class P, template <typename> class A, typename T>
struct nested {
  template <typename U> using pointer_constructor = P<U>;
  template <typename U> using array_constructor = A<U>;
  typedef T value_type;
  P<A<T>> data;
  template <typename Archive> void serialize(Archive &ar) { ar(data); }
};
}

#define ADT_NESTED_HPP_DONE
#endif // #ifdef ADT_NESTED_HPP
#ifndef ADT_NESTED_HPP_DONE
#error "Cyclic include dependency"
#endif
