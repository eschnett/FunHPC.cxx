#ifndef ADT_TREE_HPP
#define ADT_TREE_HPP

namespace adt {

template <template <typename> class C, typename T> class tree {
  // data Tree a = Node [(Tree a, a)] | Empty
  // data Tree a = Node (C (Tree a, a)) | Empty

  struct node {
    tree subtree;
    T elt;
  };
  C<node> branches;

public:
  tree() : branches(fun::mzero<C, node>()) {}
  tree(const T &x) : branches(fun::munit<C>(node{tree(), x})) {}
  tree(T &&x) : branches(fun::munit<C>(node{tree(), std::move(x)})) {}

  bool empty() const { return branches.empty(); }
};
}

#define ADT_TREE_HPP_DONE
#endif // #ifdef ADT_TREE_HPP
#ifndef ADT_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
