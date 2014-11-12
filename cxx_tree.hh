#ifndef CXX_TREE_HH
#define CXX_TREE_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_either.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_ostreaming.hh"
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

// A tree is either a leaf or a branch, a branch is container of
// pointers to trees, and a leaf is a container of pointers to values.

template <typename T, template <typename> class C, template <typename> class P>
class leaf;
template <typename T, template <typename> class C, template <typename> class P>
class branch;
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

// ostreaming

template <typename T, template <typename> class C, template <typename> class P>
struct tree_output;

template <typename T, template <typename> class C, template <typename> class P>
auto output(const tree<T, C, P> &xs) -> cxx::ostreaming<std::tuple<> > {
  return tree_output<T, C, P>::output(xs);
}

// foldable

template <typename F, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldable;

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const tree<T, C, P> &xs, const As &... as) {
  return tree_foldable<F, rpc::is_action<F>::value, R, T, C, P, As...>::foldl(
      f, z, xs, as...);
}

template <typename F, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
struct tree_foldable2;

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const tree<T, C, P> &xs,
       const tree<T2, C, P> &ys, const As &... as) {
  return tree_foldable2<F, rpc::is_action<F>::value, R, T, C, P, T2,
                        As...>::foldl2(f, z, xs, ys, as...);
}

// iota

template <typename F, bool is_action, template <typename> class tree_,
          typename... As>
struct tree_iota;

template <template <typename> class tree_, typename F, typename... As,
          typename R = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_tree<tree_<R> >::value, tree_<R> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  return tree_iota<F, rpc::is_action<F>::value, tree_, As...>::iota(f, range,
                                                                    as...);
}

// functor

template <typename F, bool is_action, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_functor;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
tree<R, C, P> fmap(const F &f, const tree<T, C, P> &xs, const As &... as) {
  return tree_functor<F, rpc::is_action<F>::value, T, C, P, As...>::fmap(f, xs,
                                                                         as...);
}

template <typename F, bool is_action, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_functor2;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
tree<R, C, P> fmap2(const F &f, const tree<T, C, P> &xs,
                    const tree<T2, C, P> &ys, const As &... as) {
  return tree_functor2<F, rpc::is_action<F>::value, T, C, P, T2, As...>::fmap2(
      f, xs, ys, as...);
}

template <typename F, typename G, bool is_action, typename T,
          template <typename> class C, template <typename> class P, typename B>
struct tree_stencil_functor;

template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B,
          typename R = typename cxx::invoke_of<F, T, B, B>::type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value,
    tree<R, C, P> >::type
stencil_fmap(const F &f, const G &g, const tree<T, C, P> &xs, const P<B> &bm,
             const P<B> &bp) {
  return tree_stencil_functor<F, G, rpc::is_action<F>::value, T, C, P,
                              B>::stencil_fmap(f, g, xs, bm, bp);
}

// Leaf

template <typename T, template <typename> class C, template <typename> class P>
class leaf {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class leaf;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class branch;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class tree;

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend struct tree_output;

  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_foldable2;

  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_functor;
  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_functor2;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B>
  friend struct tree_stencil_functor;

  C<P<T> > values;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(values); }

public:
  typedef T value_type;
  template <typename U> using container = C<U>;
  template <typename U> using pointer = P<U>;
  typedef leaf<T, C, P> leaf_t;
  typedef branch<T, C, P> branch_t;

  explicit leaf() {}
  explicit leaf(const C<P<T> > &vs) : values(vs) {}
  explicit leaf(C<P<T> > &&vs) : values(std::move(vs)) {}
  explicit leaf(const P<T> &v) : leaf(cxx::unit<C>(v)) {}
  explicit leaf(P<T> &&v) : leaf(cxx::unit<C>(std::move(v))) {}
  leaf(const leaf &l) : values(l.values) {}
  leaf(leaf &&l) : values(std::move(l.values)) {}

  // size
  bool empty() const { return values.empty(); }
  std::size_t size() const { return values.size(); }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
                          branch<R, C, P> >::type
  join() const {
    return branch<R, C, P>(values);
  }
};

// Branch

template <typename T, template <typename> class C, template <typename> class P>
class branch {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class leaf;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class branch;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class tree;

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend struct tree_output;

  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_foldable2;

  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_functor;
  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_functor2;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B>
  friend struct tree_stencil_functor;

  C<P<tree<T, C, P> > > trees;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(trees); }

public:
  typedef T value_type;
  template <typename U> using container = C<U>;
  template <typename U> using pointer = P<U>;
  typedef leaf<T, C, P> leaf_t;
  typedef branch<T, C, P> branch_t;

  explicit branch() {}
  explicit branch(const C<P<tree<T, C, P> > > &ts) : trees(ts) {}
  explicit branch(C<P<tree<T, C, P> > > &&ts) : trees(std::move(ts)) {}
  explicit branch(const P<tree<T, C, P> > &t) : branch(cxx::unit<C>(t)) {}
  explicit branch(P<tree<T, C, P> > &&t) : branch(cxx::unit<C>(std::move(t))) {}
  branch(const branch &b) : trees(b.trees) {}
  branch(branch &&b) : trees(std::move(b.trees)) {}

  // size
  bool empty() const {
    return cxx::foldl([](bool is_empty, const P<tree<T, C, P> > &t) {
                        return cxx::foldl(
                            [](bool is_empty, const tree<T, C, P> &t) {
                              return is_empty && t.empty();
                            },
                            true, t);
                      },
                      true, trees);
  }
  std::size_t size() const {
    return cxx::foldl([](std::size_t size, const P<tree<T, C, P> > &t) {
                        return cxx::foldl(
                            [](std::size_t size, const tree<T, C, P> &t) {
                              return size + t.size();
                            },
                            std::size_t(0), t);
                      },
                      std::size_t(0), trees);
  }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
                          branch<R, C, P> >::type
  join() const {
    return branch<R, C, P>(cxx::fmap(
        [](const P<tree<T, C, P> > &pt) {
          return cxx::fmap([](const tree<T, C, P> &t) { return t.join(); }, pt);
        },
        trees));
  }
};

// Tree

template <typename T, template <typename> class C, template <typename> class P>
class tree {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class leaf;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class branch;
  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class tree;

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend struct tree_output;

  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_foldable2;

  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_functor;
  template <typename F, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename T2, typename... As>
  friend struct tree_functor2;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B>
  friend struct tree_stencil_functor;

public:
  typedef T value_type;
  template <typename U> using container = C<U>;
  template <typename U> using pointer = P<U>;
  typedef leaf<T, C, P> leaf_t;
  typedef branch<T, C, P> branch_t;

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef const T &const_reference;
  typedef const T *const_pointer;

private:
  typedef cxx::either<leaf<T, C, P>, branch<T, C, P> > node_t;
  node_t node;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(node); }

public:
  explicit tree() : node(leaf<T, C, P>()) {}
  explicit tree(const leaf<T, C, P> &l) : node(l) {}
  explicit tree(leaf<T, C, P> &&l) : node(std::move(l)) {}
  explicit tree(const branch<T, C, P> &b) : node(b) {}
  explicit tree(branch<T, C, P> &&b) : node(std::move(b)) {}
  explicit tree(const node_t &n) : node(n) {}
  explicit tree(node_t &&n) : node(std::move(n)) {}
  explicit tree(const P<T> &pv) : tree(leaf<T, C, P>(pv)) {}
  explicit tree(P<T> &&pv) : tree(leaf<T, C, P>(std::move(pv))) {}
  explicit tree(const T &v) : tree(cxx::unit<P>(v)) {}
  explicit tree(T &&v) : tree(cxx::unit<P>(std::move(v))) {}
  tree(const tree &t) : node(t.node) {}
  tree(tree &&t) : node(std::move(t.node)) {}
  tree &operator=(const tree &t) {
    node = t.node;
    return *this;
  }
  tree &operator=(tree &&t) {
    node = std::move(t.node);
    return *this;
  }

  bool empty() const {
    return node.gfoldl([](const leaf<T, C, P> &l) { return l.empty(); },
                       [](const branch<T, C, P> &b) { return b.empty(); });
  }
  std::size_t size() const {
    return node.gfoldl([](const leaf<T, C, P> &l) { return l.size(); },
                       [](const branch<T, C, P> &b) { return b.size(); });
  }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
                          tree<R, C, P> >::type
  join() const {
    return tree<R, C, P>(node.is_left() ? node.left().join()
                                        : node.right().join());
  }

  // plus
  struct plus : std::tuple<> {};
  tree(plus, const P<tree> &x, const P<tree> &y)
      : tree(branch<T, C, P>(cxx::some<C>(x, y))) {}
  tree(plus, const tree &x, const tree &y)
      : tree(plus(), cxx::unit<P>(x), cxx::unit<P>(y)) {}

  // size
  bool empty_slow() const {
    return foldl([](bool, const T &v) { return false; }, true, *this);
  }
  std::size_t size_slow() const {
    return foldl([](std::size_t s, const T &v) { return s + 1; },
                 std::size_t(0), *this);
  }
};

////////////////////////////////////////////////////////////////////////////////

// output

template <typename T, template <typename> class C, template <typename> class P>
struct tree_output {

  typedef leaf<T, C, P> leaf;
  typedef branch<T, C, P> branch;
  typedef tree<T, C, P> tree;

  // TODO: add indentation to ostreaming?
  // static auto indent(int level) -> cxx::ostreaming<std::tuple<> > {
  //   return cxx::put(ostreamer() << std::string(2 * level, ' '));
  // }
  static auto indent(int level) -> std::string {
    return std::string(2 * level, ' ');
  }

  static auto output_pvalue(const P<T> &pv, int level)
      -> cxx::ostreaming<std::tuple<> > {
    return cxx::put(ostreamer() << indent(level) << "value\n");
  }

  static auto output_leaf(const leaf &l, int level)
      -> cxx::ostreaming<std::tuple<> > {
    return cxx::put(cxx::ostreamer() << indent(level) << "leaf(nvalues="
                                     << l.values.size() << ")\n") >>
           cxx::mapM_(output_pvalue, l.values, level + 1);
  }

  static auto output_ptree(const P<tree> &pt, int level)
      -> cxx::ostreaming<std::tuple<> > {
    return cxx::mapM_(output_tree_action(), pt, level);
  }

  static auto output_branch(const branch &b, int level)
      -> cxx::ostreaming<std::tuple<> > {
    return cxx::put(cxx::ostreamer() << indent(level) << "branch(ntrees="
                                     << b.trees.size() << ")\n") >>
           cxx::mapM_(output_ptree, b.trees, level + 1);
  }

  static auto output_tree(const tree &t, int level)
      -> cxx::ostreaming<std::tuple<> > {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(output_tree);
    return cxx::gfoldl(output_leaf, output_branch, t.node, level);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(output_tree);

  static auto output(const tree &xs) -> cxx::ostreaming<std::tuple<> > {
    // TODO: output without structure?
    return bind0(
        cxx::put(cxx::ostreamer() << "tree[" << typeid(xs).name() << "]\n"),
        output_tree(xs, 0));
  }
};
// Define action exports
template <typename T, template <typename> class C, template <typename> class P>
typename tree_output<T, C, P>::output_tree_evaluate_export_t
    tree_output<T, C, P>::output_tree_evaluate_export =
        output_tree_evaluate_export_init();
template <typename T, template <typename> class C, template <typename> class P>
typename tree_output<T, C, P>::output_tree_finish_export_t
    tree_output<T, C, P>::output_tree_finish_export =
        output_tree_finish_export_init();

// foldable

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_foldable<F, false, R, T, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
      "");

  // TODO: These don't need to be templates
  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldl_pvalue(const R &z, const P<T> &pv, const F &f,
                        const As &... as) {
    return cxx::foldl(f, z, pv, as...);
  }

  static R foldl_leaf(const leaf<T> &l, const R &z, const F &f,
                      const As &... as) {
    return cxx::foldl(foldl_pvalue, z, l.values, f, as...);
  }

  static R foldl_ptree(const R &z, const P<tree<T> > &pt, const F &f,
                       const As &... as) {
    return cxx::foldl(foldl_tree, z, pt, f, as...);
  }

  static R foldl_branch(const branch<T> &b, const R &z, const F &f,
                        const As &... as) {
    return cxx::foldl(foldl_ptree, z, b.trees, f, as...);
  }

  static R foldl_tree(const R &z, const tree<T> &t, const F &f,
                      const As &... as) {
    return cxx::gfoldl(foldl_leaf, foldl_branch, t.node, z, f, as...);
  }

  static R foldl(const F &f, const R &z, const tree<T> &t, const As &... as) {
    return foldl_tree(z, t, f, as...);
  }
};

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_foldable<F, true, R, T, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
      "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldl_pvalue(const R &z, const P<T> &pv, const As &... as) {
    return cxx::foldl(F(), z, pv, as...);
  }

  static R foldl_leaf(const leaf<T> &l, const R &z, const As &... as) {
    return cxx::foldl(foldl_pvalue, z, l.values, as...);
  }

  static R foldl_ptree(const R &z, const P<tree<T> > &pt, const As &... as) {
    return cxx::foldl(foldl_tree_action(), z, pt, as...);
  }

  static R foldl_branch(const branch<T> &b, const R &z, const As &... as) {
    return cxx::foldl(foldl_ptree, z, b.trees, as...);
  }

  static R foldl_tree(const R &z, const tree<T> &t, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_tree);
    return cxx::gfoldl(foldl_leaf, foldl_branch, t.node, z, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_tree);

  static R foldl(F, const R &z, const tree<T> &t, const As &... as) {
    return foldl_tree(z, t, as...);
  }
};
// Define action exports
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_foldable<F, true, R, T, C, P, As...>::foldl_tree_evaluate_export_t
    tree_foldable<F, true, R, T, C, P, As...>::foldl_tree_evaluate_export =
        foldl_tree_evaluate_export_init();
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_foldable<F, true, R, T, C, P, As...>::foldl_tree_finish_export_t
    tree_foldable<F, true, R, T, C, P, As...>::foldl_tree_finish_export =
        foldl_tree_finish_export_init();

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_foldable2<F, false, R, T, C, P, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
      "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldl2_pvalue(const R &z, const P<T> &pv, const P<T2> &pv2,
                         const F &f, const As &... as) {
    return cxx::foldl2(f, z, pv, pv2, as...);
  }

  static R foldl2_leaf(const leaf<T> &l, const leaf<T2> &l2, const R &z,
                       const F &f, const As &... as) {
    return cxx::foldl2(foldl2_pvalue, z, l.values, l2.values, f, as...);
  }

  static R foldl2_ptree(const R &z, const P<tree<T> > &pt,
                        const P<tree<T2> > &pt2, const F &f, const As &... as) {
    return cxx::foldl2(foldl2_tree, z, pt, pt2, f, as...);
  }

  static R foldl2_branch(const branch<T> &b, const branch<T2> &b2, const R &z,
                         const F &f, const As &... as) {
    return cxx::foldl2(foldl2_ptree, z, b.trees, b2.trees, f, as...);
  }

  static R foldl2_tree(const R &z, const tree<T> &t, const tree<T2> &t2,
                       const F &f, const As &... as) {
    return cxx::gfoldl2(foldl2_leaf, foldl2_branch, t.node, t2.node, z, f,
                        as...);
  }

  static R foldl2(const F &f, const R &z, const tree<T> &t, const tree<T2> &t2,
                  const As &... as) {
    return foldl2_tree(z, t, t2, f, as...);
  }
};

template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_foldable2<F, true, R, T, C, P, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
      "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldl2_pvalue(const R &z, const P<T> &pv, const P<T2> &pv2,
                         const As &... as) {
    return cxx::foldl2(F(), z, pv, pv2, as...);
  }

  static R foldl2_leaf(const leaf<T> &l, const leaf<T2> &l2, const R &z,
                       const As &... as) {
    return cxx::foldl2(foldl2_pvalue, z, l.values, l2.values, as...);
  }

  static R foldl2_ptree(const R &z, const P<tree<T> > &pt,
                        const P<tree<T2> > &pt2, const As &... as) {
    return cxx::foldl2(foldl2_tree_action(), z, pt, pt2, as...);
  }

  static R foldl2_branch(const branch<T> &b, const branch<T2> &b2, const R &z,
                         const As &... as) {
    return cxx::foldl2(foldl2_ptree, z, b.trees, b2.trees, as...);
  }

  static R foldl2_tree(const R &z, const tree<T> &t, const tree<T2> &t2,
                       const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_tree);
    return cxx::gfoldl2(foldl2_leaf, foldl2_branch, t.node, t2.node, z, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_tree);

  static R foldl2(F, const R &z, const tree<T> &t, const tree<T2> &t2,
                  const As &... as) {
    return foldl2_tree(z, t, as...);
  }
};
// Define action exports
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
typename tree_foldable2<F, true, R, T, C, P, T2,
                        As...>::foldl2_tree_evaluate_export_t
    tree_foldable2<F, true, R, T, C, P, T2,
                   As...>::foldl2_tree_evaluate_export =
        foldl2_tree_evaluate_export_init();
template <typename F, typename R, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
typename tree_foldable2<F, true, R, T, C, P, T2,
                        As...>::foldl2_tree_finish_export_t
    tree_foldable2<F, true, R, T, C, P, T2, As...>::foldl2_tree_finish_export =
        foldl2_tree_finish_export_init();

// iota

template <typename F, template <typename> class tree, typename... As>
struct tree_iota<F, false, tree, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type R;

  template <typename U> using leaf = typename tree<U>::leaf_t;
  template <typename U> using branch = typename tree<U>::branch_t;

  template <typename U> using C = typename tree<U>::template container<U>;
  template <typename U> using P = typename tree<U>::template pointer<U>;

  static constexpr std::ptrdiff_t max_node_size = 10;

  static P<R> iota_pvalue(std::ptrdiff_t i, const F &f,
                          const iota_range_t &range, const As &... as) {
    iota_range_t range1(range.global,
                        { i, std::min(range.local.imax, i + range.local.istep),
                          range.global.istep });
    assert(range1.local.size() == 1);
    return cxx::iota<P>(f, range1, as...);
  }

  static leaf<R> iota_leaf(const F &f, const iota_range_t &range,
                           const As &... as) {
    // TODO: handle empty case
    assert(!range.local.empty() && range.local.size() <= max_node_size);
    iota_range_t range1(range.global, { range.local.imin, range.local.imax,
                                        range.global.istep });
    return leaf<R>(cxx::iota<C>(iota_pvalue, range1, f, range1, as...));
  }

  static P<tree<R> > iota_ptree(std::ptrdiff_t i, const F &f,
                                const iota_range_t &range, const As &... as) {
    iota_range_t range1(range.global,
                        { i, std::min(range.local.imin, i + range.local.istep),
                          range.global.istep });
    assert(range1.local.size() == 1);
    iota_range_t range2(range.global,
                        { i, std::min(range.local.imax, i + range.local.istep),
                          range.global.istep });
    return cxx::iota<P>(iota_tree, range1, f, range2, as...);
  }

  static branch<R> iota_branch(const F &f, const iota_range_t &range,
                               const As &... as) {
    assert(!range.local.empty() && range.local.size() > max_node_size);
    iota_range_t range1(
        range.global,
        { range.local.imin, range.local.imax,
          cxx::align_ceil(
              cxx::div_ceil(range.local.imax - range.local.imin, max_node_size),
              range.global.istep) });
    assert(!range1.local.empty() && range1.local.size() <= max_node_size);
    return branch<R>(cxx::iota<C>(iota_ptree, range1, f, range1, as...));
  }

  static tree<R> iota_tree(std::ptrdiff_t i, const F &f,
                           const iota_range_t &range, const As &... as) {
    assert(i == range.local.imin);
    if (range.local.size() <= max_node_size) {
      return tree<R>(iota_leaf(f, range, as...));
    } else {
      return tree<R>(iota_branch(f, range, as...));
    }
  }

  static tree<R> iota(const F &f, const iota_range_t &range, const As &... as) {
    return iota_tree(range.local.imin, f, range, as...);
  }
};

template <typename F, template <typename> class tree, typename... As>
struct tree_iota<F, true, tree, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type R;

  // template <typename U> using leaf = leaf<U, C, P>;
  // template <typename U> using branch = branch<U, C, P>;
  // template <typename U> using tree = tree<U, C, P>;
  template <typename U> using leaf = typename tree<U>::leaf_t;
  template <typename U> using branch = typename tree<U>::branch_t;

  template <typename U> using C = typename tree<U>::template container<U>;
  template <typename U> using P = typename tree<U>::template pointer<U>;

  static constexpr std::ptrdiff_t max_node_size = 10;

  static P<R> iota_pvalue(std::ptrdiff_t i, const iota_range_t &range,
                          const As &... as) {
    iota_range_t range1(range.global,
                        { i, std::min(range.local.imax, i + range.local.istep),
                          range.global.istep });
    assert(range1.local.size() == 1);
    return cxx::iota<P>(F(), range1, as...);
  }

  static leaf<R> iota_leaf(const iota_range_t &range, const As &... as) {
    // TODO: handle empty case
    assert(!range.local.empty() && range.local.size() <= max_node_size);
    iota_range_t range1(range.global, { range.local.imin, range.local.imax,
                                        range.global.istep });
    return leaf<R>(cxx::iota<C>(iota_pvalue, range1, range1, as...));
  }

  static P<tree<R> > iota_ptree(std::ptrdiff_t i, const iota_range_t &range,
                                const As &... as) {
    iota_range_t range1(range.global,
                        { i, std::min(range.local.imax, i + range.global.istep),
                          range.global.istep });
    assert(range1.local.size() == 1);
    iota_range_t range2(range.global,
                        { i, std::min(range.local.imax, i + range.local.istep),
                          range.global.istep });
    return cxx::iota<P>(iota_tree, range1, range2, as...);
  }

  static branch<R> iota_branch(const iota_range_t &range, const As &... as) {
    assert(!range.local.empty() && range.local.size() > max_node_size);
    iota_range_t range1(
        range.global,
        { range.local.imin, range.local.imax,
          cxx::align_ceil(
              cxx::div_ceil(range.local.imax - range.local.imin, max_node_size),
              range.global.istep) });
    assert(!range1.local.empty() && range1.local.size() <= max_node_size);
    return branch<R>(cxx::iota<C>(iota_ptree, range1, range1, as...));
  }

  static tree<R> iota_tree(std::ptrdiff_t i, const iota_range_t &range,
                           const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);
    assert(i == range.local.imin);
    if (range.local.size() <= max_node_size) {
      return tree<R>(iota_leaf(range, as...));
    } else {
      return tree<R>(iota_branch(range, as...));
    }
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);

  static tree<R> iota(const F &f, const iota_range_t &range, const As &... as) {
    return iota_tree(range.local.imin, range, as...);
  }
};
// Define action exports
template <typename F, template <typename> class tree, typename... As>
typename tree_iota<F, true, tree, As...>::iota_tree_evaluate_export_t
    tree_iota<F, true, tree, As...>::iota_tree_evaluate_export =
        iota_tree_evaluate_export_init();
template <typename F, template <typename> class tree, typename... As>
typename tree_iota<F, true, tree, As...>::iota_tree_finish_export_t
    tree_iota<F, true, tree, As...>::iota_tree_finish_export =
        iota_tree_finish_export_init();

// functor

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_functor<F, false, T, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, As...>::type R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static P<R> fmap_pvalue(const P<T> &pv, const F &f, const As &... as) {
    return cxx::fmap(f, pv, as...);
  }

  static leaf<R> fmap_leaf(const leaf<T> &l, const F &f, const As &... as) {
    return leaf<R>(cxx::fmap(fmap_pvalue, l.values, f, as...));
  }

  static P<tree<R> > fmap_ptree(const P<tree<T> > &pt, const F &f,
                                const As &... as) {
    return cxx::fmap(fmap_tree, pt, f, as...);
  }

  static branch<R> fmap_branch(const branch<T> &b, const F &f,
                               const As &... as) {
    return branch<R>(cxx::fmap(fmap_ptree, b.trees, f, as...));
  }

  static tree<R> fmap_tree(const tree<T> &t, const F &f, const As &... as) {
    return tree<R>(cxx::gmap(fmap_leaf, fmap_branch, t.node, f, as...));
  }

  static tree<R> fmap(const F &f, const tree<T> &xs, const As &... as) {
    return fmap_tree(xs, f, as...);
  }
};

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_functor<F, true, T, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, As...>::type R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static P<R> fmap_pvalue(const P<T> &pv, const As &... as) {
    return cxx::fmap(F(), pv, as...);
  }

  static leaf<R> fmap_leaf(const leaf<T> &l, const As &... as) {
    return leaf<R>(cxx::fmap(fmap_pvalue, l.values, as...));
  }

  static P<tree<R> > fmap_ptree(const P<tree<T> > &pt, const As &... as) {
    return cxx::fmap(fmap_tree_action(), pt, as...);
  }

  static branch<R> fmap_branch(const branch<T> &b, const As &... as) {
    return branch<R>(cxx::fmap(fmap_ptree, b.trees, as...));
  }

  static tree<R> fmap_tree(const tree<T> &t, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_tree);
    return tree<R>(cxx::gmap(fmap_leaf, fmap_branch, t.node, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_tree);

  static tree<R> fmap(F, const tree<T> &xs, const As &... as) {
    return fmap_tree(xs, as...);
  }
};
// Define action exports
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_functor<F, true, T, C, P, As...>::fmap_tree_evaluate_export_t
    tree_functor<F, true, T, C, P, As...>::fmap_tree_evaluate_export =
        fmap_tree_evaluate_export_init();
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_functor<F, true, T, C, P, As...>::fmap_tree_finish_export_t
    tree_functor<F, true, T, C, P, As...>::fmap_tree_finish_export =
        fmap_tree_finish_export_init();

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_functor2<F, false, T, C, P, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static P<R> fmap2_pvalue(const P<T> &pv, const P<T2> &pv2, const F &f,
                           const As &... as) {
    return cxx::fmap2(f, pv, pv2, as...);
  }

  static leaf<R> fmap2_leaf(const leaf<T> &l, const leaf<T2> &l2, const F &f,
                            const As &... as) {
    return leaf<R>(cxx::fmap2(fmap2_pvalue, l.values, l2.values, f, as...));
  }

  static P<tree<R> > fmap2_ptree(const P<tree<T> > &pt, const P<tree<T2> > &pt2,
                                 const F &f, const As &... as) {
    return cxx::fmap2(fmap2_tree, pt, pt2, f, as...);
  }

  static branch<R> fmap2_branch(const branch<T> &b, const branch<T2> &b2,
                                const F &f, const As &... as) {
    return branch<R>(cxx::fmap2(fmap2_ptree, b.trees, b2.trees, f, as...));
  }

  static tree<R> fmap2_tree(const tree<T> &t, const tree<T2> &t2, const F &f,
                            const As &... as) {
    return tree<R>(
        cxx::gmap2(fmap2_leaf, fmap2_branch, t.node, t2.node, f, as...));
  }

  static tree<R> fmap2(const F &f, const tree<T> &xs, const tree<T2> &ys,
                       const As &... as) {
    return fmap2_tree(xs, ys, f, as...);
  }
};

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_functor2<F, true, T, C, P, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static P<R> fmap2_pvalue(const P<T> &pv, const P<T2> &pv2, const As &... as) {
    return cxx::fmap2(F(), pv, pv2, as...);
  }

  static leaf<R> fmap2_leaf(const leaf<T> &l, const leaf<T2> &l2,
                            const As &... as) {
    return leaf<R>(cxx::fmap2(fmap2_pvalue, l.values, l2.values, as...));
  }

  static P<tree<R> > fmap2_ptree(const P<tree<T> > &pt, const P<tree<T2> > &pt2,
                                 const As &... as) {
    return cxx::fmap2(fmap2_tree_action(), pt, pt2, as...);
  }

  static branch<R> fmap2_branch(const branch<T> &b, const branch<T2> &b2,
                                const As &... as) {
    return branch<R>(cxx::fmap2(fmap2_ptree, b.trees, b2.trees, as...));
  }

  static tree<R> fmap2_tree(const tree<T> &t, const tree<T2> &t2,
                            const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_tree);
    return tree<R>(
        cxx::gmap2(fmap2_leaf, fmap2_branch, t.node, t2.node, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_tree);

  static tree<R> fmap2(F, const tree<T> &xs, const tree<T2> &ys,
                       const As &... as) {
    return fmap2_tree(xs, ys, as...);
  }
};
// Define action exports
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
typename tree_functor2<F, true, T, C, P, T2,
                       As...>::fmap2_tree_evaluate_export_t
    tree_functor2<F, true, T, C, P, T2, As...>::fmap2_tree_evaluate_export =
        fmap2_tree_evaluate_export_init();
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
typename tree_functor2<F, true, T, C, P, T2, As...>::fmap2_tree_finish_export_t
    tree_functor2<F, true, T, C, P, T2, As...>::fmap2_tree_finish_export =
        fmap2_tree_finish_export_init();

template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B>
struct tree_stencil_functor<F, G, true, T, C, P, B> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<G>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value, "");

  typedef typename cxx::invoke_of<F, T, B, B>::type R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static leaf<R> stencil_fmap_leaf(const leaf<T> &l, const P<B> &bm,
                                   const P<B> &bp) {
    std::size_t s = l.values.size();
    C<P<R> > values(s);
    assert(s > 0);
    if (s == 1) {
      values[0] = cxx::fmap3(F(), l.values[0], bm, bp);
    } else {
      values[0] =
          cxx::fmap3(F(), l.values[0], bm, cxx::fmap(G(), l.values[1], false));
      for (std::size_t i = 1; i < s - 1; ++i) {
        values[i] =
            cxx::fmap3(F(), l.values[i], cxx::fmap(G(), l.values[i - 1], true),
                       cxx::fmap(G(), l.values[i + 1], false));
      }
      values[s - 1] = cxx::fmap3(F(), l.values[s - 1],
                                 cxx::fmap(G(), l.values[s - 2], true), bp);
    }
    return leaf<R>(std::move(values));
  }

  static branch<R> stencil_fmap_branch(const branch<T> &b, const P<B> &bm,
                                       const P<B> &bp) {
    // TODO: use array<...,N>, and allow multiple ghost zones
    // TODO: add push_back equivalent to constructor
    // TODO: for efficiency: don't use push_back, use something else
    // that requires preallocation, that doesn't reallocate, and which
    // turns into an indexed loop when used for vectors
    std::size_t s = b.trees.size();
    C<P<tree<R> > > trees(s);
    assert(s > 0);
    if (s == 1) {
      trees[0] = cxx::fmap(stencil_fmap_tree_action(), b.trees[0], bm, bp);
    } else {
      trees[0] =
          cxx::fmap(stencil_fmap_tree_action(), b.trees[0], bm,
                    cxx::bind(b.trees[1], get_boundary_tree_action(), false));
      for (std::size_t i = 1; i < s - 1; ++i) {
        trees[i] = cxx::fmap(
            stencil_fmap_tree_action(), b.trees[i],
            cxx::bind(b.trees[i - 1], get_boundary_tree_action(), true),
            cxx::bind(b.trees[i + 1], get_boundary_tree_action(), false));
      }
      trees[s - 1] = cxx::fmap(
          stencil_fmap_tree_action(), b.trees[s - 1],
          cxx::bind(b.trees[s - 2], get_boundary_tree_action(), true), bp);
    }
    return branch<R>(std::move(trees));
  }

  static tree<R> stencil_fmap_tree(const tree<T> &t, const P<B> &bm,
                                   const P<B> &bp) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);
    return tree<R>(
        cxx::gmap(stencil_fmap_leaf, stencil_fmap_branch, t.node, bm, bp));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);

  static tree<R> stencil_fmap(F, G, const tree<T> &xs, const P<B> &bm,
                              const P<B> &bp) {
    return stencil_fmap_tree(xs, bm, bp);
  }

  static P<B> get_boundary_leaf(const leaf<T> &l, bool face_upper) {
    assert(!l.values.empty());
    return cxx::fmap(G(), !face_upper ? l.values.front() : l.values.back(),
                     face_upper);
  }

  static P<B> get_boundary_branch(const branch<T> &b, bool face_upper) {
    assert(!b.trees.empty());
    return cxx::bind(!face_upper ? b.trees.front() : b.trees.back(),
                     get_boundary_tree_action(), face_upper);
  }

  static P<B> get_boundary_tree(const tree<T> &t, bool face_upper) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
    return cxx::gfoldl(get_boundary_leaf, get_boundary_branch, t.node,
                       face_upper);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);

  static P<B> get_boundary(G, const tree<T> &xs, bool face_upper) {
    return get_boundary_tree(xs, face_upper);
  }
};
// Define action exports
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B>
typename tree_stencil_functor<F, G, true, T, C, P,
                              B>::stencil_fmap_tree_evaluate_export_t
    tree_stencil_functor<F, G, true, T, C, P,
                         B>::stencil_fmap_tree_evaluate_export =
        stencil_fmap_tree_evaluate_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B>
typename tree_stencil_functor<F, G, true, T, C, P,
                              B>::stencil_fmap_tree_finish_export_t
    tree_stencil_functor<F, G, true, T, C, P,
                         B>::stencil_fmap_tree_finish_export =
        stencil_fmap_tree_finish_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B>
typename tree_stencil_functor<F, G, true, T, C, P,
                              B>::get_boundary_tree_evaluate_export_t
    tree_stencil_functor<F, G, true, T, C, P,
                         B>::get_boundary_tree_evaluate_export =
        get_boundary_tree_evaluate_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B>
typename tree_stencil_functor<F, G, true, T, C, P,
                              B>::get_boundary_tree_finish_export_t
    tree_stencil_functor<F, G, true, T, C, P,
                         B>::get_boundary_tree_finish_export =
        get_boundary_tree_finish_export_init();

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type unit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type
make(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, template <typename> class C1, template <typename> class P,
          typename CCT = cxx::tree<cxx::tree<T, C1, P>, C1, P>,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const cxx::tree<cxx::tree<T, C1, P>, C1, P> &xss) {
  return xss.join();
}

// TODO: Implement this directly
template <typename T, template <typename> class C1, template <typename> class P,
          typename F, typename... As, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> bind(const cxx::tree<T, C1, P> &xs, const F &f, const As &... as) {
  return join(fmap(f, xs, as...));
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type zero() {
  return C<T>();
}

template <typename T, template <typename> class C1, template <typename> class P,
          typename... As, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
plus(const cxx::tree<T, C1, P> &xs, const As &... as) {
  return C<T>(typename C<T>::plus(), xs, as...);
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<((cxx::is_tree<C<T> >::value) &&
                         (cxx::all<std::is_same<As, T>::value...>::value)),
                        C<T> >::type
some(T &&x, As &&... as) {
  // TODO: Implement directly
  return plus(unit<C>(std::forward<T>(x)), unit<C>(std::forward<As>(as))...);
}
}

#endif // #ifndef CXX_TREE_HH
