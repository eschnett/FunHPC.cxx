// A tree that stores elements in the branches instead of leaves. I
// now think that this implementation, while likely shorter, is not as
// elegant and won't have performance benefits.

#ifndef CXX_TREE_HH
#define CXX_TREE_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_either.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_utils.hh"

#include <cereal/access.hpp>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace cxx {

// A tree class, which is a distributed version of a vector

template <typename T, template <typename> class C, template <typename> class P>
class tree;

// kinds

template <typename T, template <typename> class C, template <typename> class P>
struct kinds<tree<T, C, P> > {
  typedef T value_type;
  template <typename U> using constructor = tree<U, C, P>;
};
template <typename T> struct is_tree : std::false_type {};
template <typename T, template <typename> class C, template <typename> class P>
struct is_tree<tree<T, C, P> > : std::true_type {};

// foldable

template <typename Op, typename R, template <typename> class C,
          template <typename> class P>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value, R>::type
fold(const Op &op, const R &z, const tree<R, C, P> &xs);

template <typename F, typename Op, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldMap;

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const tree<T, C, P> &xs,
        const As &... as) {
  return tree_foldMap<F, Op, rpc::is_action<F>::value, R, T, C, P,
                      As...>::foldMap_tree(xs, f, op, z, as...);
}

// iota

template <typename F, bool is_action, typename R,
          template <typename> class tree_, typename... As>
struct tree_iota;

template <template <typename> class tree_, typename F, typename... As,
          typename R = typename cxx::invoke_of<F, std::ptrdiff_t, iota_range_t,
                                               As...>::type>
typename std::enable_if<cxx::is_tree<tree_<R> >::value, tree_<R> >::type
iota(const F &f, const iota_range_t &range, const iota_range_t &grange,
     const As &... as) {
  return tree_iota<F, rpc::is_action<F>::value, R, tree_, As...>::iota(
      f, range, grange, as...);
}

// functor

template <typename F, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_fmap;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
tree<R, C, P> fmap(const F &f, const tree<T, C, P> &xs, const As &... as) {
  return tree_fmap<F, rpc::is_action<F>::value, R, T, C, P, As...>::fmap_tree(
      f, xs, as...);
}

template <typename F, bool is_action, typename R, typename T, typename T2,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_fmap2;

template <typename F, typename T, typename T2, template <typename> class C,
          template <typename> class P, typename... As,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
tree<R, C, P> fmap2(const F &f, const tree<T, C, P> &xs,
                    const tree<T2, C, P> &ys, const As &... as) {
  return tree_fmap2<F, rpc::is_action<F>::value, R, T, T2, C, P,
                    As...>::fmap2_tree(f, xs, ys, as...);
}

template <typename F, typename G, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P, typename B,
          typename... As>
struct tree_stencil_fmap;

template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, typename... As,
          typename R = typename cxx::invoke_of<F, T, B, B, As...>::type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value,
    tree<R, C, P> >::type
stencil_fmap(const F &f, const G &g, const tree<T, C, P> &xs, const P<B> &bm,
             const P<B> &bp, const As &... as) {
  return tree_stencil_fmap<F, G, rpc::is_action<F>::value, R, T, C, P,
                           B>::stencil_fmap_tree(f, g, xs, bm, bp, as...);
}

////////////////////////////////////////////////////////////////////////////////

// Tree

template <typename T, template <typename> class C, template <typename> class P>
class tree {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class tree;

  template <typename F, typename Op, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldMap;

  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_fmap;

  template <typename F, bool is_action, typename R, typename T1, typename T2,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_fmap2;

  template <typename F, typename G, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B, typename... As>
  friend struct tree_stencil_fmap;

public:
  typedef T value_type;
  template <typename U> using container = C<U>;
  template <typename U> using pointer = P<U>;

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef const T &const_reference;
  typedef const T *const_pointer;

private:
  static constexpr std::ptrdiff_t max_node_size = 10;

  struct branch {
    P<T> value;
    P<tree> subtree;

  private:
    friend class cereal::access;
    template <typename Archive> void serialize(Archive &ar) {
      ar(value, subtree);
    }
  };
  C<branch> branches;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(branches); }

public:
  bool invariant() const {
    for (const auto &b : branches) {
      // Subtrees cannot be empty; empty subtrees are stored as null
      // pointers
      if (b.subtree)
        if (b.subtree->empty())
          return false;
      // All values must be present
      if (!b.value)
        return false;
    }
    return true;
  }

  // empty tree
  explicit tree() {}

  // one-element tree from value
  explicit tree(const T &v) : branches({ { cxx::munit<P>(v), P<tree>() } }) {}
  explicit tree(T &&v)
      : branches({ { cxx::munit<P>(std::move(v)), P<tree>() } }) {}

  // one-element tree from pointer to value
  explicit tree(const P<T> &pv) : branches({ { pv, P<tree>() } }) {}
  explicit tree(P<T> &&pv) : branches({ { std::move(pv), P<tree>() } }) {}

  // copy constructor etc.
  tree(const tree &t) : branches(t.branches) {}
  tree(tree &&t) : tree() { swap(t); }
  void swap(tree &t) { std::swap(branches, t.branches); }
  tree &operator=(const tree &t) {
    branches = t.branches;
    return *this;
  }
  tree &operator=(tree &&t) { return *this = tree().swap(t); }

  bool empty() const { return branches.empty(); }
  std::size_t size() const {
    return foldlMap([](const T &) { return 1; },
                    [](std::size_t x, std::size_t y) { return x + y; }, 0,
                    *this);
  }

  // // monad: mjoin
  // template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  // typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
  //                         tree<R, C, P> >::type
  // mjoin() const {
  //   return tree<R, C, P>(node.is_left() ? node.left().mjoin()
  //                                       : node.right().mjoin());
  // }
  //
  // // mplus
  // struct mplus : std::tuple<> {};
  // tree(mplus, const P<tree> &x, const P<tree> &y)
  //     : tree(branch<T, C, P>(cxx::msome<C>(x, y))) {}
  // tree(mplus, const tree &x, const tree &y)
  //     : tree(mplus(), cxx::munit<P>(x), cxx::munit<P>(y)) {}
};

template <typename T, template <typename> class C, template <typename> class P>
inline void swap(tree<T, C, P> &t1, tree<T, C, P> &t2) {
  t1.swap(t2);
}

////////////////////////////////////////////////////////////////////////////////

// output

// template <typename T, template <typename> class C, template <typename> class
// P>
// struct tree_output {
//
//   typedef leaf<T, C, P> leaf;
//   typedef branch<T, C, P> branch;
//   typedef tree<T, C, P> tree;
//
//   // TODO: add indentation to ostreaming?
//   // static auto indent(int level) -> cxx::ostreaming<std::tuple<> > {
//   //   return cxx::put(ostreamer() << std::string(2 * level, ' '));
//   // }
//   static auto indent(int level) -> std::string {
//     return std::string(2 * level, ' ');
//   }
//
//   static auto output_pvalue(const P<T> &pv, int level)
//       -> cxx::ostreaming<std::tuple<> > {
//     return cxx::put(ostreamer() << indent(level) << "value\n");
//   }
//
//   static auto output_leaf(const leaf &l, int level)
//       -> cxx::ostreaming<std::tuple<> > {
//     return cxx::put(cxx::ostreamer() << indent(level) << "leaf(nvalues="
//                                      << l.values.size() << ")\n") >>
//            cxx::mapM_(output_pvalue, l.values, level + 1);
//   }
//
//   static auto output_ptree(const P<tree> &pt, int level)
//       -> cxx::ostreaming<std::tuple<> > {
//     return cxx::mapM_(output_tree_action(), pt, level);
//   }
//
//   static auto output_branch(const branch &b, int level)
//       -> cxx::ostreaming<std::tuple<> > {
//     return cxx::put(cxx::ostreamer() << indent(level) << "branch(ntrees="
//                                      << b.trees.size() << ")\n") >>
//            cxx::mapM_(output_ptree, b.trees, level + 1);
//   }
//
//   static auto output_tree(const tree &t, int level)
//       -> cxx::ostreaming<std::tuple<> > {
//     RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(output_tree);
//     return cxx::gfoldl(output_leaf, output_branch, t.node, level);
//   }
//   RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(output_tree);
//
//   static auto output(const tree &xs) -> cxx::ostreaming<std::tuple<> > {
//     // TODO: output without structure?
//     return mbind0(
//         cxx::put(cxx::ostreamer() << "tree[" << typeid(xs).name() << "]\n"),
//         output_tree(xs, 0));
//   }
// };
// // Define action exports
// template <typename T, template <typename> class C, template <typename> class
// P>
// typename tree_output<T, C, P>::output_tree_evaluate_export_t
//     tree_output<T, C, P>::output_tree_evaluate_export =
//         output_tree_evaluate_export_init();
// template <typename T, template <typename> class C, template <typename> class
// P>
// typename tree_output<T, C, P>::output_tree_finish_export_t
//     tree_output<T, C, P>::output_tree_finish_export =
//         output_tree_finish_export_init();

// foldable

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldMap<F, Op, false, R, T, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  template <typename U> using tree = tree<U, C, P>;

  static R foldMap_branch(const typename tree<R>::branch &b, const F &f,
                          const Op &op, const R &z, const As &... as) {
    R r1 = foldMap(f, op, z, b.value, as...);
    R r2 = foldMap(foldMap_tree, op, z, b.subtree, f, op, z, as...);
    return cxx::invoke(op, std::move(r1), std::move(r2));
  }

  static R foldMap_tree(const tree<R> &t, const F &f, const Op &op, const R &z,
                        const As &... as) {
    return foldMap(foldMap_branch, op, z, t.branches, f, op, z, as...);
  }
};

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldMap<F, Op, true, R, T, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  template <typename U> using tree = tree<U, C, P>;

  static R foldMap_branch(const typename tree<R>::branch &b, F f, Op op,
                          const R &z, const As &... as) {
    R r1 = foldMap(f, op, z, b.value, as...);
    R r2 = foldMap(foldMap_tree, op, z, b.subtree, f, op, z, as...);
    return cxx::invoke(op, std::move(r1), std::move(r2));
  }

  static R foldMap_tree(const tree<R> &t, F f, Op op, const R &z,
                        const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_tree);
    return foldMap(foldMap_branch, op, z, t.branches, f, op, z, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_tree);
};
// Define action exports
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_foldMap<F, Op, true, R, T, C, P,
                      As...>::foldMap_tree_evaluate_export_t
    tree_foldMap<F, Op, true, R, T, C, P, As...>::foldMap_tree_evaluate_export =
        foldMap_tree_evaluate_export_init();
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_foldMap<F, Op, true, R, T, C, P,
                      As...>::foldMap_tree_finish_export_t
    tree_foldMap<F, Op, true, R, T, C, P, As...>::foldMap_tree_finish_export =
        foldMap_tree_finish_export_init();

template <bool is_action, typename T> struct tree_fold;

template <typename T> struct tree_fold<false, T> {
  static T identity(const T &x) { return x; }
  static auto get_identity() { return identity; }
};

template <typename T> struct tree_fold<true, T> {
  static T identity(const T &x) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(identity);
    return x;
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(identity);
  static auto get_identity() { return identity_action(); }
};
template <typename T>
typename tree_fold<true, T>::identity_evaluate_export_t
    tree_fold<true, T>::identity_evaluate_export =
        identity_evaluate_export_init();
template <typename T>
typename tree_fold<true, T>::identity_finish_export_t
    tree_fold<true, T>::identity_finish_export = identity_finish_export_init();

template <typename Op, typename R, template <typename> class C,
          template <typename> class P>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value, R>::type
fold(const Op &op, const R &z, const tree<R, C, P> &xs) {
  return foldMap(tree_fold<rpc::is_action<Op>::value, R>::get_identity(), op, z,
                 xs);
}

// iota

template <typename F, typename R, template <typename> class tree,
          typename... As>
struct tree_iota<F, false, R, tree, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  static typename tree<R>::branch iota_branch(std::ptrdiff_t i,
                                              const iota_range_t &grange,
                                              const F &f, std::ptrdiff_t count,
                                              const As &... as) {
    assert(count >= 1);
    // Create pointer to value
    P<R> value;
    {
      iota_range_t my_range(i, i + grange.istep, grange.istep);
      assert(my_range.size() == 1);
      value = iota<P>(f, my_range, grange, as...);
    }
    // Create pointer to subtree
    P<tree<R> > subtree;
    {
      std::ptrdiff_t sub_count = count - 1;
      if (sub_count == 0) {
        // Leaves are null pointers
        subtree = mzero<P>();
      } else {
        std::ptrdiff_t i1 = i + grange.istep;
        iota_range_t my_range(i1, i1 + grange.istep, grange.istep);
        assert(my_range.size() == 1);
        subtree = iota<P>(iota_tree, my_range, grange, f, sub_count, as...);
      }
    }
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> iota_tree(std::ptrdiff_t i, const iota_range_t &grange,
                           const F &f, std::ptrdiff_t count, const As &... as) {
    if (count == 0)
      // Emptry tree
      return tree<R>();
    // Choose how many elements to store at this level
    // Here, we prefer full subtrees, and a partially filled root
    std::ptrdiff_t subtree_sz = 0, max_sz = max_node_size;
    while (max_sz < count) {
      subtree_sz = max_sz;
      // Each level stores max_node_size elements and max_node_size
      // subtrees
      max_sz = max_node_size * (1 + subtree_sz);
    }
    // We now put subtree_sz+1 elements into each branch (1 element
    // directly into the branch, and subtree_sz elements into the
    // subtree)
    std::ptrdiff_t sub_count = subtree_sz + 1;
    iota_range_t my_range(i,
                          std::min(grange.imax, i + sub_count * grange.istep),
                          sub_count * grange.istep);
    assert(!my_range.empty());
    assert(my_range.size() <= max_node_size);
    return iota<C>(iota_branch, my_range, grange, f, count, as...);
  }
};

template <typename F, typename R, template <typename> class tree,
          typename... As>
struct tree_iota<F, true, R, tree, As...> {
  static_assert(rpc::is_action<F>::value, "");

  static typename tree<R>::branch iota_branch(std::ptrdiff_t i,
                                              const iota_range_t &grange, F f,
                                              std::ptrdiff_t count,
                                              const As &... as) {
    assert(count >= 1);
    // Create pointer to value
    P<R> value;
    {
      iota_range_t my_range(i, i + grange.istep, grange.istep);
      assert(my_range.size() == 1);
      value = iota<P>(f, my_range, grange, as...);
    }
    // Create pointer to subtree
    P<tree<R> > subtree;
    {
      std::ptrdiff_t sub_count = count - 1;
      if (sub_count == 0) {
        // Leaves are null pointers
        subtree = mzero<P>();
      } else {
        std::ptrdiff_t i1 = i + grange.istep;
        iota_range_t my_range(i1, i1 + grange.istep, grange.istep);
        assert(my_range.size() == 1);
        subtree = iota<P>(iota_tree, my_range, grange, f, sub_count, as...);
      }
    }
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> iota_tree(std::ptrdiff_t i, const iota_range_t &grange, F f,
                           std::ptrdiff_t count, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);
    if (count == 0)
      // Emptry tree
      return tree<R>();
    // Choose how many elements to store at this level
    // Here, we prefer full subtrees, and a partially filled root
    std::ptrdiff_t subtree_sz = 0, max_sz = max_node_size;
    while (max_sz < count) {
      subtree_sz = max_sz;
      // Each level stores max_node_size elements and max_node_size
      // subtrees
      max_sz = max_node_size * (1 + subtree_sz);
    }
    // We now put subtree_sz+1 elements into each branch (1 element
    // directly into the branch, and subtree_sz elements into the
    // subtree)
    std::ptrdiff_t sub_count = subtree_sz + 1;
    iota_range_t my_range(i,
                          std::min(grange.imax, i + sub_count * grange.istep),
                          sub_count * grange.istep);
    assert(!my_range.empty());
    assert(my_range.size() <= max_node_size);
    return iota<C>(iota_branch, my_range, grange, f, count, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);
};
// Define action exports
template <typename F, typename R, template <typename> class tree,
          typename... As>
typename tree_iota<F, true, R, tree, As...>::iota_tree_evaluate_export_t
    tree_iota<F, true, R, tree, As...>::iota_tree_evaluate_export =
        iota_tree_evaluate_export_init();
template <typename F, typename R, template <typename> class tree,
          typename... As>
typename tree_iota<F, true, R, tree, As...>::iota_tree_finish_export_t
    tree_iota<F, true, R, tree, As...>::iota_tree_finish_export =
        iota_tree_finish_export_init();

// functor

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_fmap<F, false, R, T, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");

  template <typename U> using tree = tree<U, C, P>;

  static typename tree<R>::branch fmap_branch(const typename tree<T>::branch &b,
                                              const F &f, const As &... as) {
    P<R> value = fmap(f, b.value, as...);
    P<tree<R> > subtree = fmap(fmap_tree, b.subtree, f, as...);
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> fmap_tree(const tree<T> &t, const F &f, const As &... as) {
    return fmap(fmap_branch, t.branches, f, as...);
  }
};

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_fmap<F, true, R, T, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");

  template <typename U> using tree = tree<U, C, P>;

  static typename tree<R>::branch fmap_branch(const typename tree<R>::branch &b,
                                              F f, const As &... as) {
    P<R> value = fmap(f, b.value, as...);
    P<tree<R> > subtree = fmap(fmap_tree_action(), b.subtree, f, as...);
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> fmap_tree(const tree<T> &t, F f, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_tree);
    return fmap(fmap_branch, t.branches, f, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_tree);
};
// Define action exports
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_fmap<F, true, R, T, C, P, As...>::fmap_tree_evaluate_export_t
    tree_fmap<F, true, R, T, C, P, As...>::fmap_tree_evaluate_export =
        fmap_tree_evaluate_export_init();
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_fmap<F, true, R, T, C, P, As...>::fmap_tree_finish_export_t
    tree_fmap<F, true, R, T, C, P, As...>::fmap_tree_finish_export =
        fmap_tree_finish_export_init();

template <typename F, typename R, typename T, typename T2,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_fmap2<F, false, R, T, T2, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value,
      "");

  template <typename U> using tree = tree<U, C, P>;

  static typename tree<R>::branch
  fmap2_branch(const typename tree<T>::branch &b,
               const typename tree<T2>::branch &b2, const F &f,
               const As &... as) {
    P<R> value = fmap2(f, b.value, b2.value, as...);
    P<tree<R> > subtree = fmap2(fmap2_tree, b.subtree, b2.subtree, f, as...);
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> fmap2_tree(const tree<T> &t, const tree<T2> &t2, const F &f,
                            const As &... as) {
    return fmap2(fmap2_branch, t.branches, t2.branches, f, as...);
  }
};

template <typename F, typename R, typename T, typename T2,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_fmap2<F, true, R, T, T2, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value,
      "");

  template <typename U> using tree = tree<U, C, P>;

  static typename tree<R>::branch
  fmap2_branch(const typename tree<T>::branch &b,
               const typename tree<T2>::branch &b2, F f, const As &... as) {
    P<R> value = fmap2(f, b.value, b2.value, as...);
    P<tree<R> > subtree =
        fmap2(fmap2_tree_action(), b.subtree, b2.subtree, f, as...);
    return { std::move(value), std::move(subtree) };
  }

  static tree<R> fmap2_tree(const tree<T> &t, const tree<T2> &t2, F f,
                            const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_tree);
    return fmap2(fmap2_branch, t.branches, t2.branches, f, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_tree);
};
// Define action exports
template <typename F, typename R, typename T, typename T2,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_fmap2<F, true, R, T, T2, C, P,
                    As...>::fmap2_tree_evaluate_export_t
    tree_fmap2<F, true, R, T, T2, C, P, As...>::fmap2_tree_evaluate_export =
        fmap2_tree_evaluate_export_init();
template <typename F, typename R, typename T, typename T2,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_fmap2<F, true, R, T, T2, C, P, As...>::fmap2_tree_finish_export_t
    tree_fmap2<F, true, R, T, T2, C, P, As...>::fmap2_tree_finish_export =
        fmap2_tree_finish_export_init();

template <typename F, typename G, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P, typename B,
          typename... As>
struct tree_stencil_fmap<F, G, false, R, T, C, P, B, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(!rpc::is_action<G>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, B, B, As...>::type, R>::value,
      "");
  static_assert(
      std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value, "");

  template <typename U> using tree = tree<U, C, P>;

  stencil_fmap_branch(const typename tree<T>::branch &b, const P<B> &bm,
                      const P<B> &bp, ...) {
    CONTINUE HERE
  }
  stencil_boundary_branch(const typename tree<T>::branch &b, bool face_upper) {
    if (!face_upper) {
      return cxx::invoke(g, b.value, face_upper);
    } else {
      if (!b.subtree)
        return cxx::invoke(g, b.value, face_upper);
      return stencil_boundary_tree(g, b.subtree, face_upper);
    }
  }

  static tree<R, C, P> stencil_fmap_tree(const F &f, const G &g,
                                         const tree<T> &xs, const P<B> &bm,
                                         const P<B> &bp, const As &... as) {
    assert(!xs.empty());
    return stencil_fmap(stencil_fmap_branch, stencil_boundary_branch,
                        xs.branches, vm, bp, as...);
  }
};

// template <typename F, typename G, typename T, template <typename> class C,
//           template <typename> class P, typename B>
// struct tree_stencil_functor<F, G, true, T, C, P, B> {
//   static_assert(rpc::is_action<F>::value, "");
//   static_assert(rpc::is_action<G>::value, "");
//   static_assert(
//       std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value, "");
//
//   typedef typename cxx::invoke_of<F, T, B, B>::type R;
//
//   template <typename U> using leaf = leaf<U, C, P>;
//   template <typename U> using branch = branch<U, C, P>;
//   template <typename U> using tree = tree<U, C, P>;
//
//   static leaf<R> stencil_fmap_leaf(const leaf<T> &l, const P<B> &bm,
//                                    const P<B> &bp) {
//     std::size_t s = l.values.size();
//     C<P<R> > values(s);
//     assert(s > 0);
//     if (s == 1) {
//       values[0] = cxx::fmap3(F(), l.values[0], bm, bp);
//     } else {
//       values[0] =
//           cxx::fmap3(F(), l.values[0], bm, cxx::fmap(G(), l.values[1],
//           false));
//       for (std::size_t i = 1; i < s - 1; ++i) {
//         values[i] =
//             cxx::fmap3(F(), l.values[i], cxx::fmap(G(), l.values[i - 1],
//             true),
//                        cxx::fmap(G(), l.values[i + 1], false));
//       }
//       values[s - 1] = cxx::fmap3(F(), l.values[s - 1],
//                                  cxx::fmap(G(), l.values[s - 2], true), bp);
//     }
//     return leaf<R>(std::move(values));
//   }
//
//   static branch<R> stencil_fmap_branch(const branch<T> &b, const P<B> &bm,
//                                        const P<B> &bp) {
//     // TODO: use array<...,N>, and allow multiple ghost zones
//     // TODO: add push_back equivalent to constructor
//     // TODO: for efficiency: don't use push_back, use msomething else
//     // that requires preallocation, that doesn't reallocate, and which
//     // turns into an indexed loop when used for vectors
//     std::size_t s = b.trees.size();
//     C<P<tree<R> > > trees(s);
//     assert(s > 0);
//     if (s == 1) {
//       trees[0] = cxx::fmap(stencil_fmap_tree_action(), b.trees[0], bm, bp);
//     } else {
//       trees[0] =
//           cxx::fmap(stencil_fmap_tree_action(), b.trees[0], bm,
//                     cxx::mbind(b.trees[1], get_boundary_tree_action(),
//                     false));
//       for (std::size_t i = 1; i < s - 1; ++i) {
//         trees[i] = cxx::fmap(
//             stencil_fmap_tree_action(), b.trees[i],
//             cxx::mbind(b.trees[i - 1], get_boundary_tree_action(), true),
//             cxx::mbind(b.trees[i + 1], get_boundary_tree_action(), false));
//       }
//       trees[s - 1] = cxx::fmap(
//           stencil_fmap_tree_action(), b.trees[s - 1],
//           cxx::mbind(b.trees[s - 2], get_boundary_tree_action(), true), bp);
//     }
//     return branch<R>(std::move(trees));
//   }
//
//   static tree<R> stencil_fmap_tree(const tree<T> &t, const P<B> &bm,
//                                    const P<B> &bp) {
//     RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);
//     return tree<R>(
//         cxx::gmap(stencil_fmap_leaf, stencil_fmap_branch, t.node, bm, bp));
//   }
//   RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);
//
//   static tree<R> stencil_fmap(F, G, const tree<T> &xs, const P<B> &bm,
//                               const P<B> &bp) {
//     return stencil_fmap_tree(xs, bm, bp);
//   }
//
//   static P<B> get_boundary_leaf(const leaf<T> &l, bool face_upper) {
//     assert(!l.values.empty());
//     return cxx::fmap(G(), !face_upper ? l.values.front() : l.values.back(),
//                      face_upper);
//   }
//
//   static P<B> get_boundary_branch(const branch<T> &b, bool face_upper) {
//     assert(!b.trees.empty());
//     return cxx::mbind(!face_upper ? b.trees.front() : b.trees.back(),
//                      get_boundary_tree_action(), face_upper);
//   }
//
//   static P<B> get_boundary_tree(const tree<T> &t, bool face_upper) {
//     RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
//     return cxx::gfoldl(get_boundary_leaf, get_boundary_branch, t.node,
//                        face_upper);
//   }
//   RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
//
//   static P<B> get_boundary(G, const tree<T> &xs, bool face_upper) {
//     return get_boundary_tree(xs, face_upper);
//   }
// };
// // Define action exports
// template <typename F, typename G, typename T, template <typename> class C,
//           template <typename> class P, typename B>
// typename tree_stencil_functor<F, G, true, T, C, P,
//                               B>::stencil_fmap_tree_evaluate_export_t
//     tree_stencil_functor<F, G, true, T, C, P,
//                          B>::stencil_fmap_tree_evaluate_export =
//         stencil_fmap_tree_evaluate_export_init();
// template <typename F, typename G, typename T, template <typename> class C,
//           template <typename> class P, typename B>
// typename tree_stencil_functor<F, G, true, T, C, P,
//                               B>::stencil_fmap_tree_finish_export_t
//     tree_stencil_functor<F, G, true, T, C, P,
//                          B>::stencil_fmap_tree_finish_export =
//         stencil_fmap_tree_finish_export_init();
// template <typename F, typename G, typename T, template <typename> class C,
//           template <typename> class P, typename B>
// typename tree_stencil_functor<F, G, true, T, C, P,
//                               B>::get_boundary_tree_evaluate_export_t
//     tree_stencil_functor<F, G, true, T, C, P,
//                          B>::get_boundary_tree_evaluate_export =
//         get_boundary_tree_evaluate_export_init();
// template <typename F, typename G, typename T, template <typename> class C,
//           template <typename> class P, typename B>
// typename tree_stencil_functor<F, G, true, T, C, P,
//                               B>::get_boundary_tree_finish_export_t
//     tree_stencil_functor<F, G, true, T, C, P,
//                          B>::get_boundary_tree_finish_export =
//         get_boundary_tree_finish_export_init();

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type munit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

// template <typename T, template <typename> class C1, template <typename> class
// P,
//           typename CCT = cxx::tree<cxx::tree<T, C1, P>, C1, P>,
//           template <typename> class C = cxx::kinds<CCT>::template
//           constructor,
//           typename CT = typename cxx::kinds<CCT>::value_type,
//           template <typename> class C2 = cxx::kinds<CT>::template
//           constructor>
// C<T> mjoin(const cxx::tree<cxx::tree<T, C1, P>, C1, P> &xss) {
//   return xss.mjoin();
// }
//
// // TODO: Implement this directly
// template <typename T, template <typename> class C1, template <typename> class
// P,
//           typename F, typename... As, typename CT = cxx::tree<T, C1, P>,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename CR = typename cxx::invoke_of<F, T, As...>::type,
//           typename R = typename cxx::kinds<CR>::value_type>
// C<R> mbind(const cxx::tree<T, C1, P> &xs, const F &f, const As &... as) {
//   return mjoin(fmap(f, xs, as...));
// }

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}

// template <typename T, template <typename> class C1, template <typename> class
// P,
//           typename... As, typename CT = cxx::tree<T, C1, P>,
//           template <typename> class C = cxx::kinds<CT>::template constructor>
// typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
//                         C<T> >::type
// mplus(const cxx::tree<T, C1, P> &xs, const As &... as) {
//   return C<T>(typename C<T>::mplus(), xs, as...);
// }
//
// template <template <typename> class C, typename T, typename... As>
// typename std::enable_if<((cxx::is_tree<C<T> >::value) &&
//                          (cxx::all<std::is_same<As, T>::value...>::value)),
//                         C<T> >::type
// msome(T &&x, As &&... as) {
//   // TODO: Implement directly
//   return mplus(munit<C>(std::forward<T>(x)),
//   munit<C>(std::forward<As>(as))...);
// }
}

#endif // #ifndef CXX_TREE_HH
