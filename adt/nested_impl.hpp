#ifndef ADT_NESTED_IMPL_HPP
#define ADT_NESTED_IMPL_HPP

#include "nested_decl.hpp"

#include <adt/dummy.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <cstddef>
#include <type_traits>

namespace adt {

namespace detail {
template <typename T> struct nested_default_policy : std::tuple<> {
  // std::size_t(-1) means unlimited
  constexpr std::size_t min_outer_size() const noexcept { return 0; }
  constexpr std::size_t max_outer_size() const noexcept { return -1; }
  constexpr std::size_t min_inner_size() const noexcept { return 0; }
  constexpr std::size_t max_inner_size() const noexcept { return -1; }
  template <typename U> struct rebind {
    typedef nested_default_policy<U> other;
  };
  // TODO: try to remove these default definition, also in wave1d.cc
  nested_default_policy() = default;
  nested_default_policy(const nested_default_policy &other) = default;
  nested_default_policy(nested_default_policy &&other) = default;
  nested_default_policy &
  operator=(const nested_default_policy &other) = default;
  nested_default_policy &operator=(nested_default_policy &&other) = default;
  template <typename U>
  nested_default_policy(const nested_default_policy<U> &other) {}
};
}

template <typename P, typename A, typename T, typename Policy> struct nested {
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
  typedef Policy policy_type;

  pointer_constructor<array_constructor<T>> data;

  policy_type policy;
  constexpr policy_type get_policy() const noexcept { return policy; }

  template <typename Archive> void serialize(Archive &ar) { ar(data, policy); }

  nested() = default;
  nested(const nested &) = default;
  nested(nested &&) = default;
  nested &operator=(const nested &) = default;
  nested &operator=(nested &&) = default;
};
template <typename P, typename A, typename T, typename Policy>
void swap(nested<P, A, T, Policy> &x, nested<P, A, T, Policy> &y) {
  swap(x.data, y.data);
}
}

#define ADT_NESTED_IMPL_HPP_DONE
#endif // #ifdef ADT_NESTED_IMPL_HPP
#ifndef ADT_NESTED_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
