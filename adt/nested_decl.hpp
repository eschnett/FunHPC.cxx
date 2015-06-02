#ifndef ADT_NESTED_DECL_HPP
#define ADT_NESTED_DECL_HPP

namespace adt {

template <typename P, typename A, typename T> struct nested;
template <typename P, typename A, typename T>
void swap(nested<P, A, T> &x, nested<P, A, T> &y);
}

#define ADT_NESTED_DECL_HPP_DONE
#endif // #ifdef ADT_NESTED_DECL_HPP
#ifndef ADT_NESTED_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
