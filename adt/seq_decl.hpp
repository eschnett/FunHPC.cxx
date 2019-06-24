#ifndef ADT_SEQ_DECL_HPP
#define ADT_SEQ_DECL_HPP

namespace adt {

template <typename A, typename B, typename T> struct seq;

template <typename A, typename B, typename T>
void swap(seq<A, B, T> &x, seq<A, B, T> &y);
} // namespace adt

#define ADT_SEQ_DECL_HPP_DONE
#endif // #ifdef ADT_SEQ_DECL_HPP
#ifndef ADT_SEQ_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
