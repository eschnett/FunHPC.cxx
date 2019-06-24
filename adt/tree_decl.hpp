#ifndef ADT_TREE_DECL_HPP
#define ADT_TREE_DECL_HPP

namespace adt {

template <typename A, typename T> struct tree;
template <typename A, typename T> void swap(tree<A, T> &x, tree<A, T> &y);
} // namespace adt

#define ADT_TREE_DECL_HPP_DONE
#endif // #ifdef ADT_TREE_DECL_HPP
#ifndef ADT_TREE_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
