#ifndef CXX_TUPLE_HPP
#define CXX_TUPLE_HPP

#include <cxx/utility.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace cxx {

template <typename T, std::size_t N>
using ntuple = decltype(std::tuple_cat(std::declval<std::array<T, N>>()));

// Tuple subset

// Note: All Indices must be distinct
template <typename Tuple, std::size_t... Indices>
auto tuple_subset(Tuple &&tuple, std::index_sequence<Indices...>) {
  return std::make_tuple(std::get<Indices>(std::forward<Tuple>(tuple))...);
}

// Tuple section

template <std::ptrdiff_t start, std::ptrdiff_t count, std::ptrdiff_t stride = 1,
          typename Tuple, std::enable_if_t<(count <= 0)> * = nullptr>
auto tuple_section(Tuple &&tuple) {
  return std::tuple<>();
}

template <std::ptrdiff_t start, std::ptrdiff_t count, std::ptrdiff_t stride = 1,
          typename Tuple, std::enable_if_t<(count > 0)> * = nullptr>
auto tuple_section(Tuple &&tuple) {
  static_assert(stride != 0, "");
  constexpr std::ptrdiff_t size = std::tuple_size<std::decay_t<Tuple>>::value;
  static_assert(start >= 0 && start < size, "");
  constexpr std::ptrdiff_t last = start + (count - 1) * stride;
  static_assert(last >= 0 && last < size, "");
  return tuple_subset(
      std::forward<Tuple>(tuple),
      cxx::affine_map<start, stride>(std::make_index_sequence<count>()));
}

// Convert tuple to array

namespace detail {
template <typename T, std::size_t N, typename Tuple, std::size_t... Indices>
auto to_array(Tuple &&tuple, std::index_sequence<Indices...>) {
  return std::array<T, N>{{std::get<Indices>(std::forward<Tuple>(tuple))...}};
}
} // namespace detail

template <typename Tuple> auto to_array(Tuple &&tuple) {
  constexpr size_t N = std::tuple_size<Tuple>::value;
  static_assert(N > 0, "");
  typedef std::decay_t<std::tuple_element_t<0, Tuple>> T;
  return detail::to_array<T, N>(std::forward<Tuple>(tuple),
                                std::make_index_sequence<N>());
}
} // namespace cxx

#define CXX_TUPLE_HPP_DONE
#endif // #ifdef CXX_TUPLE_HPP
#ifndef CXX_TUPLE_HPP_DONE
#error "Cyclic include dependency"
#endif
