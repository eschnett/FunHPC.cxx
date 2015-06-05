#ifndef ADT_PAR_DECL_HPP
#define ADT_PAR_DECL_HPP

namespace adt {

template <typename A, typename B, typename T> struct par;

template <typename A, typename B, typename T>
void swap(par<A, B, T> &x, par<A, B, T> &y);
}

#define ADT_PAR_DECL_HPP_DONE
#endif // #ifdef ADT_PAR_DECL_HPP
#ifndef ADT_PAR_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
