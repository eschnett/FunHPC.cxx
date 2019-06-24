#ifndef CXX_APPLY_HPP
#define CXX_APPLY_HPP

#include <cxx/invoke.hpp>

#include <tuple>
#include <utility>

namespace cxx {

namespace detail {
template <typename F, typename Tuple, size_t... I>
decltype(auto) apply_impl(F &&f, Tuple &&t, std::index_sequence<I...>) {
  return cxx::invoke(std::forward<F>(f),
                     std::get<I>(std::forward<Tuple>(t))...);
}
} // namespace detail

template <typename F, typename Tuple> decltype(auto) apply(F &&f, Tuple &&t) {
  using Indices =
      std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
  return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                            Indices{});
}
} // namespace cxx

#define CXX_APPLY_HPP_DONE
#endif // #ifdef CXX_APPLY_HPP
#ifndef CXX_APPLY_HPP_DONE
#error "Cyclic include dependency"
#endif
