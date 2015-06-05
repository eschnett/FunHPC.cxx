#ifndef ADT_NESTED_DECL_HPP
#define ADT_NESTED_DECL_HPP

namespace adt {

namespace detail {
template <typename T> struct nested_default_policy;
}

template <typename P, typename A, typename T,
          typename Policy = detail::nested_default_policy<T>>
struct nested;

template <typename P, typename A, typename T, typename Policy>
void swap(nested<P, A, T, Policy> &x, nested<P, A, T, Policy> &y);
}

#define ADT_NESTED_DECL_HPP_DONE
#endif // #ifdef ADT_NESTED_DECL_HPP
#ifndef ADT_NESTED_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
