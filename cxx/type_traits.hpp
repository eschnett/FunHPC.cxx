#ifndef CXX_TYPE_TRAITS_HPP
#define CXX_TYPE_TRAITS_HPP

namespace cxx {
template <bool C> struct error_if { static_assert(C, ""); };
} // namespace cxx

#define CXX_TYPE_TRAITS_HPP_DONE
#endif // #ifdef CXX_TYPE_TRAITS_HPP
#ifndef CXX_TYPE_TRAITS_HPP_DONE
#error "Cyclic include dependency"
#endif
