#ifndef CXX_INVOKE_HPP
#define CXX_INVOKE_HPP

#include <utility>

namespace cxx {

// internal declarations from libc++ 3.4

struct __any {
  __any(...);
};

struct __nat {
  __nat() = delete;
  __nat(const __nat &) = delete;
  __nat &operator=(const __nat &) = delete;
  ~__nat() = delete;
};

// member_pointer_traits, taken from libc++ 3.4

template <class _MP, bool _IsMemberFuctionPtr, bool _IsMemberObjectPtr>
struct member_pointer_traits_imp {};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...), true, false> {
  typedef _Class class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const, true,
                                 false> {
  typedef _Class const class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile, true,
                                 false> {
  typedef _Class volatile class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile,
                                 true, false> {
  typedef _Class const volatile class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) &, true, false> {
  typedef _Class &class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const &, true,
                                 false> {
  typedef _Class const &class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile &, true,
                                 false> {
  typedef _Class volatile &class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile &,
                                 true, false> {
  typedef _Class const volatile &class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) &&, true, false> {
  typedef _Class &&class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const &&, true,
                                 false> {
  typedef _Class const &&class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile &&, true,
                                 false> {
  typedef _Class volatile &&class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class, class... _Param>
struct member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile &&,
                                 true, false> {
  typedef _Class const volatile &&class_type;
  typedef _Rp return_type;
};

template <class _Rp, class _Class>
struct member_pointer_traits_imp<_Rp _Class::*, false, true> {
  typedef _Class class_type;
  typedef _Rp return_type;
};

template <class _MP>
struct member_pointer_traits : public member_pointer_traits_imp<
                                   typename std::remove_cv<_MP>::type,
                                   std::is_member_function_pointer<_MP>::value,
                                   std::is_member_object_pointer<_MP>::value> {
  // typedef ... class_type;
  // typedef ... return_type;
};

// check_complete, taken from libc++ 3.4

template <class... _Tp> struct check_complete;

template <> struct check_complete<> {};

template <class _Hp, class _T0, class... _Tp>
struct check_complete<_Hp, _T0, _Tp...>
    : private check_complete<_Hp>, private check_complete<_T0, _Tp...> {};

template <class _Hp>
struct check_complete<_Hp, _Hp> : private check_complete<_Hp> {};

template <class _Tp> struct check_complete<_Tp> {
  static_assert(sizeof(_Tp) > 0, "Type must be complete.");
};

template <class _Tp>
struct check_complete<_Tp &> : private check_complete<_Tp> {};

template <class _Tp>
struct check_complete<_Tp &&> : private check_complete<_Tp> {};

template <class _Rp, class... _Param>
struct check_complete<_Rp (*)(_Param...)> : private check_complete<_Rp> {};

template <class... _Param> struct check_complete<void (*)(_Param...)> {};

template <class _Rp, class... _Param>
struct check_complete<_Rp(_Param...)> : private check_complete<_Rp> {};

template <class... _Param> struct check_complete<void(_Param...)> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...)>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) volatile>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const volatile>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) &>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const &>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) volatile &>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const volatile &>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) &&>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const &&>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) volatile &&>
    : private check_complete<_Class> {};

template <class _Rp, class _Class, class... _Param>
struct check_complete<_Rp (_Class::*)(_Param...) const volatile &&>
    : private check_complete<_Class> {};

template <class _Rp, class _Class>
struct check_complete<_Rp _Class::*> : private check_complete<_Class> {};

////////////////////////////////////////////////////////////////////////////////

// decay_copy, taken from libc++ 3.4
template <typename T>
constexpr inline typename std::decay<T>::type decay_copy(T &&t) {
  return std::forward<T>(t);
}

////////////////////////////////////////////////////////////////////////////////

// invoke forward declarations, taken from libc++ 3.4

// fall back - none of the bullets

template <class... _Args> auto invoke(__any, _Args &&... __args) -> __nat;

// bullets 1 and 2

template <class _Fp, class _A0, class... _Args,
          class = typename std::enable_if<
              std::is_member_function_pointer<
                  typename std::remove_reference<_Fp>::type>::value &&
              std::is_base_of<
                  typename member_pointer_traits<
                      typename std::remove_reference<_Fp>::type>::class_type,
                  typename std::remove_reference<_A0>::type>::value>::type>
auto invoke(_Fp &&__f, _A0 &&__a0, _Args &&... __args)
    -> decltype((std::forward<_A0>(__a0).*__f)(std::forward<_Args>(__args)...));

template <class _Fp, class _A0, class... _Args,
          class = typename std::enable_if<
              std::is_member_function_pointer<
                  typename std::remove_reference<_Fp>::type>::value &&
              !std::is_base_of<
                  typename member_pointer_traits<
                      typename std::remove_reference<_Fp>::type>::class_type,
                  typename std::remove_reference<_A0>::type>::value>::type>
auto invoke(_Fp &&__f, _A0 &&__a0, _Args &&... __args)
    -> decltype(((*std::forward<_A0>(__a0)).*
                 __f)(std::forward<_Args>(__args)...));

// bullets 3 and 4

template <class _Fp, class _A0,
          class = typename std::enable_if<
              std::is_member_object_pointer<
                  typename std::remove_reference<_Fp>::type>::value &&
              std::is_base_of<
                  typename member_pointer_traits<
                      typename std::remove_reference<_Fp>::type>::class_type,
                  typename std::remove_reference<_A0>::type>::value>::type>
auto invoke(_Fp &&__f, _A0 &&__a0) -> decltype(std::forward<_A0>(__a0).*__f);

template <class _Fp, class _A0,
          class = typename std::enable_if<
              std::is_member_object_pointer<
                  typename std::remove_reference<_Fp>::type>::value &&
              !std::is_base_of<
                  typename member_pointer_traits<
                      typename std::remove_reference<_Fp>::type>::class_type,
                  typename std::remove_reference<_A0>::type>::value>::type>
auto invoke(_Fp &&__f, _A0 &&__a0) -> decltype((*std::forward<_A0>(__a0)).*__f);

// bullet 5

template <class _Fp, class... _Args>
auto invoke(_Fp &&__f, _Args &&... __args)
    -> decltype(std::forward<_Fp>(__f)(std::forward<_Args>(__args)...));

// invokable

template <class _Fp, class... _Args>
struct invokable_imp : private check_complete<_Fp> {
  typedef decltype(invoke(std::declval<_Fp>(), std::declval<_Args>()...)) type;
  static const bool value = !std::is_same<type, __nat>::value;
};

template <class _Fp, class... _Args>
struct invokable
    : public std::integral_constant<bool, invokable_imp<_Fp, _Args...>::value> {
};

// invoke_of

template <bool _Invokable, class _Fp, class... _Args>
struct invoke_of_imp // false
{};

template <class _Fp, class... _Args> struct invoke_of_imp<true, _Fp, _Args...> {
  typedef typename invokable_imp<_Fp, _Args...>::type type;
};

template <class _Fp, class... _Args>
struct invoke_of
    : public invoke_of_imp<invokable<_Fp, _Args...>::value, _Fp, _Args...> {};

template <class... _Args>
using invoke_of_t = typename invoke_of<_Args...>::type;

// invoke, taken from libc++ 3.4

// bullets 1 and 2

template <class _Fp, class _A0, class... _Args, class>
inline auto invoke(_Fp &&__f, _A0 &&__a0, _Args &&... __args)
    -> decltype((std::forward<_A0>(__a0).*
                 __f)(std::forward<_Args>(__args)...)) {
  return (std::forward<_A0>(__a0).*__f)(std::forward<_Args>(__args)...);
}

template <class _Fp, class _A0, class... _Args, class>
inline auto invoke(_Fp &&__f, _A0 &&__a0, _Args &&... __args)
    -> decltype(((*std::forward<_A0>(__a0)).*
                 __f)(std::forward<_Args>(__args)...)) {
  return ((*std::forward<_A0>(__a0)).*__f)(std::forward<_Args>(__args)...);
}

// bullets 3 and 4

template <class _Fp, class _A0, class>
inline auto invoke(_Fp &&__f, _A0 &&__a0)
    -> decltype(std::forward<_A0>(__a0).*__f) {
  return std::forward<_A0>(__a0).*__f;
}

template <class _Fp, class _A0, class>
inline auto invoke(_Fp &&__f, _A0 &&__a0)
    -> decltype((*std::forward<_A0>(__a0)).*__f) {
  return (*std::forward<_A0>(__a0)).*__f;
}

// bullet 5

template <class _Fp, class... _Args>
inline auto invoke(_Fp &&__f, _Args &&... __args)
    -> decltype(std::forward<_Fp>(__f)(std::forward<_Args>(__args)...)) {
  return std::forward<_Fp>(__f)(std::forward<_Args>(__args)...);
}
}

#define CXX_INVOKE_HPP_DONE
#endif // #ifdef CXX_INVOKE_HPP
#ifndef CXX_INVOKE_HPP_DONE
#error "Cyclic include dependency"
#endif
