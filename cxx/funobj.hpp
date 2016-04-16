#ifndef CXX_FUNOBJ_HPP
#define CXX_FUNOBJ_HPP

#include <cxx/invoke.hpp>

#include <functional>
#include <type_traits>

namespace cxx {

// Convert functions to function objects

namespace detail {
template <typename T, T F> struct funobj_impl {
  static_assert((std::is_pointer<T>::value &&
                 std::is_function<std::remove_pointer_t<T>>::value) ||
                    std::is_member_pointer<T>::value,
                "");
  typedef T type;
  static constexpr T value = F;
  template <typename... Args>
  constexpr decltype(auto) operator()(Args &&... args) const {
    return cxx::invoke(value, std::forward<Args>(args)...);
  }
};
template <typename T, T F> constexpr T funobj_impl<T, F>::value;

template <typename T, T F> struct obj_impl {
  typedef T type;
  static constexpr T value = F;
  constexpr decltype(auto) operator()() const { return value; }
};
// template <typename T, T F> constexpr T obj_impl<T, F>::value;

template <typename T, const std::remove_reference_t<T> *F> struct objref_impl {
  static_assert(std::is_lvalue_reference<T>::value, "");
  typedef T type;
  static constexpr T value = const_cast<T &>(*F);
  constexpr decltype(auto) operator()() const { return value; }
};
// template <typename T, T F> constexpr T objref_impl<T, F>::value;
}

#define CXX_FUNOBJ(...)                                                        \
  (cxx::detail::funobj_impl<std::decay_t<decltype(__VA_ARGS__)>, __VA_ARGS__>())

#define CXX_OBJ(...)                                                           \
  (cxx::detail::obj_impl<decltype(__VA_ARGS__), __VA_ARGS__>())
#define CXX_OBJREF(...)                                                        \
  (cxx::detail::objref_impl<                                                   \
      std::add_lvalue_reference_t<decltype(__VA_ARGS__)>, &__VA_ARGS__>())
}

#define CXX_FUNOBJ_HPP_DONE
#endif // #ifdef CXX_FUNOBJ_HPP
#ifndef CXX_FUNOBJ_HPP_DONE
#error "Cyclic include dependency"
#endif
