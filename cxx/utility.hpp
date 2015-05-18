#ifndef CXX_UTILITY_HPP
#define CXX_UTILITY_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cxx {

// Affine transformation of integer sequence

template <std::ptrdiff_t offset, std::ptrdiff_t scale, typename I, I... Ints>
auto affine_map(std::integer_sequence<I, Ints...>) {
  return std::integer_sequence<I, offset + scale * Ints...>();
}

// all_of_type, any_of_type, none_of_type //////////////////////////////////////

template <bool...> struct all_of_type;
template <> struct all_of_type<> : std::true_type {};
template <bool... xs> struct all_of_type<true, xs...> : all_of_type<xs...> {};
template <bool... xs> struct all_of_type<false, xs...> : std::false_type {};

template <bool... xs>
struct any_of_type : std::integral_constant<bool, !all_of_type<!xs...>::value> {
};

template <bool... xs>
struct none_of_type : std::integral_constant<bool, all_of_type<!xs...>::value> {
};

#if 0
// forward /////////////////////////////////////////////////////////////////////

template <typename... Ts>
struct is_any_function : cxx::any_of_type<std::is_function<Ts>::value...> {};

namespace detail {
template <typename T> struct function_arguments { typedef std::tuple<> types; };
template <typename R, typename... Args> struct function_arguments<R(Args...)> {
  typedef std::tuple<Args...> types;
};
template <typename R, typename... Args>
struct function_arguments<R (*)(Args...)> {
  typedef std::tuple<Args...> types;
};
template <typename R, typename T, typename... Args>
struct function_arguments<R (T::*)(Args...)> {
  typedef std::tuple<T, Args...> types;
};

template <typename> struct tuple_has_function;
template <typename... Ts>
struct tuple_has_function<std::tuple<Ts...>>
    : cxx::any_of_type<std::is_function<Ts>::value...> {};

template <typename T>
struct has_function_argument
    : tuple_has_function<
          typename function_arguments<std::remove_reference_t<T>>::types> {};
}

template <typename... Ts>
struct has_any_function_arguments
    : cxx::any_of_type<detail::has_function_argument<Ts>::value...> {};
#endif
}

#define CXX_UTILITY_HPP_DONE
#endif // #ifdef CXX_UTILITY_HPP
#ifndef CXX_UTILITY_HPP_DONE
#error "Cyclic include dependency"
#endif
