#ifndef FUN_TOPOLOGY_HPP
#define FUN_TOPOLOGY_HPP

#include <array>
#include <utility>

namespace fun {

// 1D topology

template <typename T> struct connectivity {
  typedef T value_type;
  // std::tuple<const T &, const T &> data;
  // std::tuple<T, T> data;
  // connectivity(const T &x0, const T &x1)
  //     : data(std::forward_as_tuple(x0, x1)) {}
  std::array<T, 2> data;
  template <typename T0, typename T1>
  connectivity(T0 &&x0, T1 &&x1)
      : data{{std::forward<T0>(x0), std::forward<T1>(x1)}} {}
  template <std::ptrdiff_t I> decltype(auto) get() const {
    using std::get;
    return get<I>(data);
  }
};
template <std::ptrdiff_t I, typename T>
decltype(auto) get(const connectivity<T> &conn) {
  return conn.template get<I>();
}
}

#define FUN_TOPOLOGY_HPP_DONE
#endif // #ifdef FUN_TOPOLOGY_HPP
#ifndef FUN_TOPOLOGY_HPP_DONE
#error "Cyclic include dependency"
#endif
