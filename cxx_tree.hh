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
#include "cxx_shape.hh"
#include "cxx_utils.hh"

#include <cereal/access.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
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

template <typename T, template <typename> class C, template <typename> class P>
auto tree_output1(const tree<T, C, P> &xs) {
  return xs.output();
}

// foldable

template <typename F, typename Op, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldable;

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
R foldMap(const F &f, const Op &op, const R &z, const tree<T, C, P> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return tree_foldable<F, Op, rpc::is_action<Op>::value, R, T, C, P,
                       As...>::foldMap(f, op, z, xs, as...);
}

template <typename F, typename Op, bool is_action, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
struct tree_foldable2;

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
R foldMap2(const F &f, const Op &op, const R &z, const tree<T, C, P> &xs,
           const tree<T2, C, P> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return tree_foldable2<F, Op, rpc::is_action<Op>::value, R, T, C, P, T2,
                        As...>::foldMap2(f, op, z, xs, ys, as...);
}

template <bool is_action, typename T> struct tree_fold;

template <typename Op, typename R, template <typename> class C,
          template <typename> class P, typename... As>
R fold(const Op &op, const R &z, const tree<R, C, P> &xs, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, As...>, R>::value, "");
  return foldMap(tree_fold<rpc::is_action<Op>::value, R>::get_identity(), op, z,
                 xs);
}

template <typename T, template <typename> class C, template <typename> class P>
const T &head(const tree<T, C, P> &xs) {
  return xs.head();
}
template <typename T, template <typename> class C, template <typename> class P>
const T &last(const tree<T, C, P> &xs) {
  return xs.last();
}

// iota

template <typename F, bool is_action, template <typename> class tree_,
          typename... As>
struct tree_iota;

template <template <typename> class tree_, typename F, typename... As,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, As...> >
std::enable_if_t<cxx::is_tree<tree_<R> >::value, tree_<R> >
iota(const F &f, const iota_range_t &range, const As &... as) {
  return tree_iota<F, rpc::is_action<F>::value, tree_, As...>::iota(f, range,
                                                                    as...);
}

template <typename F, std::ptrdiff_t D, bool is_action,
          template <typename> class tree_, typename... As>
struct tree_iota_grid;

template <template <typename> class tree_, typename F, std::ptrdiff_t D,
          typename... As,
          typename R = cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
          std::enable_if_t<cxx::is_tree<tree_<R> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  return tree_iota_grid<F, D, rpc::is_action<F>::value, tree_, As...>::iota(
      f, global_range, range, as...);
}

// functor

template <typename F, bool is_action, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_functor;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As,
          typename R = cxx::invoke_of_t<F, T, As...> >
tree<R, C, P> fmap(const F &f, const tree<T, C, P> &xs, const As &... as) {
  return tree_functor<F, rpc::is_action<F>::value, T, C, P, As...>::fmap(f, xs,
                                                                         as...);
}

template <typename F, bool is_action, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As>
struct tree_functor2;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename T2, typename... As,
          typename R = cxx::invoke_of_t<F, T, T2, As...> >
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
          typename R = cxx::invoke_of_t<F, T, B, B> >
tree<R, C, P> stencil_fmap(const F &f, const G &g, const tree<T, C, P> &xs,
                           const B &bm, const B &bp) {
  static_assert(std::is_same<cxx::invoke_of_t<G, T, bool>, B>::value, "");
  return tree_stencil_functor<F, G, rpc::is_action<F>::value, T, C, P,
                              B>::stencil_fmap(f, g, xs, munit<P>(bm),
                                               munit<P>(bp));
}

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_boundary;

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
auto boundary(const F &f, const tree<T, C, P> &t, std::ptrdiff_t dir, bool face,
              const As &... as) {
  return tree_boundary<F, T, C, P, As...>::boundary(f, t, dir, face, as...);
}

template <typename F, typename G, bool is_action, typename T,
          template <typename> class C, template <typename> class P, typename B,
          std::ptrdiff_t D>
struct tree_stencil_functor_boundaries;

template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
auto stencil_fmap(const F &f, const G &g, const tree<T, C, P> &xs,
                  const cxx::boundaries<tree<B, C, P>, D> &bs) {
  typedef cxx::invoke_of_t<F, T, cxx::boundaries<B, D> > R;
  static_assert(
      std::is_same<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>, B>::value, "");
  return tree_stencil_functor_boundaries<F, G, rpc::is_action<F>::value, T, C,
                                         P, B, D>::stencil_fmap(f, g, xs, bs);
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

  template <typename F, typename Op, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, typename Op, bool is_action, typename R, typename T1,
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

  template <typename F, typename T1, template <typename> class C1,
            template <typename> class P1, typename... As>
  friend struct tree_boundary;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B, std::ptrdiff_t D>
  friend struct tree_stencil_functor_boundaries;

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
  explicit leaf(const P<T> &pv) : leaf(cxx::munit<C>(pv)) {}
  explicit leaf(P<T> &&pv) : leaf(cxx::munit<C>(std::move(pv))) {}
  explicit leaf(const T &v) : leaf(cxx::munit<P>(v)) {}
  explicit leaf(T &&v) : leaf(cxx::munit<P>(std::move(v))) {}
  leaf(const leaf &l) : values(l.values) {}
  leaf(leaf &&l) : values(std::move(l.values)) {}

  // size
  bool empty() const { return values.empty(); }
  std::size_t size() const { return values.size(); }
  std::string output() const {
    std::ostringstream os;
    os << "leaf["
       << cxx::foldMap([](const P<T> &pv) {
                         std::ostringstream os;
                         os << "(p" << pv.get_proc() << ")";
                         return os.str();
                       },
                       [](const std::string &s1,
                          const std::string &s2) { return s1 + " " + s2; },
                       std::string(), values) << "]";
    return os.str();
  }

  const T &head() const { return cxx::head(cxx::head(values)); }
  const T &last() const { return cxx::last(cxx::last(values)); }

  // monad: mjoin
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  std::enable_if_t<std::is_same<T, tree<R, C, P> >::value, branch<R, C, P> >
  mjoin() const {
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

  template <typename F, typename Op, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, typename Op, bool is_action, typename R, typename T1,
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

  template <typename F, typename T1, template <typename> class C1,
            template <typename> class P1, typename... As>
  friend struct tree_boundary;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B, std::ptrdiff_t D>
  friend struct tree_stencil_functor_boundaries;

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
  explicit branch(const P<tree<T, C, P> > &pt) : branch(cxx::munit<C>(pt)) {}
  explicit branch(P<tree<T, C, P> > &&pt)
      : branch(cxx::munit<C>(std::move(pt))) {}
  explicit branch(const tree<T, C, P> &t) : branch(cxx::munit<P>(t)) {}
  explicit branch(tree<T, C, P> &&t) : branch(cxx::munit<P>(std::move(t))) {}
  branch(const branch &b) : trees(b.trees) {}
  branch(branch &&b) : trees(std::move(b.trees)) {}

  // size
  bool empty() const {
    return cxx::foldMap([](const P<tree<T, C, P> > &pt) {
                          return cxx::foldMap(
                              [](const tree<T, C, P> &t) { return t.empty(); },
                              std::logical_and<bool>(), true, pt);
                        },
                        std::logical_and<bool>(), true, trees);
  }
  std::size_t size() const {
    return cxx::foldMap([](const P<tree<T, C, P> > &pt) {
                          return cxx::foldMap(
                              [](const tree<T, C, P> &t) { return t.size(); },
                              std::plus<std::size_t>(), std::size_t(0), pt);
                        },
                        std::plus<std::size_t>(), std::size_t(0), trees);
  }
  std::string output() const {
    std::ostringstream os;
    os << "branch["
       << cxx::foldMap(
              [](const P<tree<T, C, P> > &pt) {
                std::ostringstream os;
                os << "(p" << pt.get_proc() << " "
                   << cxx::foldMap(
                          [](const tree<T, C, P> &t) { return t.output(); },
                          [](const std::string &s1,
                             const std::string &s2) { return s1 + s2; },
                          std::string(), pt.make_local()) << ")";
                return os.str();
              },
              [](const std::string &s1,
                 const std::string &s2) { return s1 + " " + s2; },
              std::string(), trees) << "]";
    return os.str();
  }

  const T &head() const { return cxx::head(cxx::head(trees)).head(); }
  const T &last() const { return cxx::last(cxx::last(trees)).last(); }

  // monad: mjoin
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  std::enable_if_t<std::is_same<T, tree<R, C, P> >::value, branch<R, C, P> >
  mjoin() const {
    return branch<R, C, P>(
        cxx::fmap([](const P<tree<T, C, P> > &pt) {
                    return cxx::fmap(
                        [](const tree<T, C, P> &t) { return t.mjoin(); }, pt);
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

  template <typename F, typename Op, bool is_action, typename R, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename... As>
  friend struct tree_foldable;
  template <typename F, typename Op, bool is_action, typename R, typename T1,
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

  template <typename F, typename T1, template <typename> class C1,
            template <typename> class P1, typename... As>
  friend struct tree_boundary;

  template <typename F, typename G, bool is_action, typename T1,
            template <typename> class C1, template <typename> class P1,
            typename B, std::ptrdiff_t D>
  friend struct tree_stencil_functor_boundaries;

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
  explicit tree(const T &v) : tree(cxx::munit<P>(v)) {}
  explicit tree(T &&v) : tree(cxx::munit<P>(std::move(v))) {}
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
  std::string output() const {
    std::ostringstream os;
    os << "tree["
       << node.gfoldl([](const leaf<T, C, P> &l) { return l.output(); },
                      [](const branch<T, C, P> &b) { return b.output(); })
       << "]";
    return os.str();
  }

  const T &head() const {
    return node.gfoldl(&leaf<T, C, P>::head, &branch<T, C, P>::head);
  }
  const T &last() const {
    return node.gfoldl(&leaf<T, C, P>::last, &branch<T, C, P>::last);
  }

  // monad: mjoin
  template <typename U = T, typename R = typename cxx::kinds<U>::value_type>
  std::enable_if_t<std::is_same<T, tree<R, C, P> >::value, tree<R, C, P> >
  mjoin() const {
    return tree<R, C, P>(node.is_left() ? node.left().mjoin()
                                        : node.right().mjoin());
  }

  // mplus
  struct mplus : std::tuple<> {};
  tree(mplus, const P<tree> &x, const P<tree> &y)
      : tree(branch<T, C, P>(cxx::msome<C>(x, y))) {}
  tree(mplus, const tree &x, const tree &y)
      : tree(mplus(), cxx::munit<P>(x), cxx::munit<P>(y)) {}

  // size
  bool empty_slow() const {
    return foldMap([](const T &v) { return false; }, std::logical_and<bool>(),
                   true, *this);
  }
  std::size_t size_slow() const {
    return foldMap([](const T &v) { return std::size_t(1); },
                   std::plus<std::size_t>(), std::size_t(0), *this);
  }
};

template <typename T, template <typename> class C, template <typename> class P>
std::ostream &operator<<(std::ostream &os, const leaf<T, C, P> &l) {
  // return os << "leaf";
  return os << l.output();
}

template <typename T, template <typename> class C, template <typename> class P>
std::ostream &operator<<(std::ostream &os, const branch<T, C, P> &b) {
  // return os << "branch";
  return os << b.output();
}

template <typename T, template <typename> class C, template <typename> class P>
std::ostream &operator<<(std::ostream &os, const tree<T, C, P> &t) {
  // return os << "tree";
  return os << t.output();
}

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
    return mbind0(
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

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldable<F, Op, false, R, T, C, P, As...> {
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldMap_pvalue(const P<T> &pv, const F &f, const Op &op, const R &z,
                          const As &... as) {
    return cxx::foldMap(f, op, z, pv, as...);
  }

  static R foldMap_leaf(const leaf<T> &l, const F &f, const Op &op, const R &z,
                        const As &... as) {
    return cxx::foldMap(foldMap_pvalue, op, z, l.values, f, op, z, as...);
  }

  static R foldMap_ptree(const P<tree<T> > &pt, const F &f, const Op &op,
                         const R &z, const As &... as) {
    return cxx::foldMap(foldMap_tree, op, z, pt, f, op, z, as...);
  }

  static R foldMap_branch(const branch<T> &b, const F &f, const Op &op,
                          const R &z, const As &... as) {
    return cxx::foldMap(foldMap_ptree, op, z, b.trees, f, op, z, as...);
  }

  static R foldMap_tree(const tree<T> &t, const F &f, const Op &op, const R &z,
                        const As &... as) {
    return cxx::gfoldl(foldMap_leaf, foldMap_branch, t.node, f, op, z, as...);
  }

  static R foldMap(const F &f, const Op &op, const R &z, const tree<T> &t,
                   const As &... as) {
    return foldMap_tree(t, f, op, z, as...);
  }
};

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
struct tree_foldable<F, Op, true, R, T, C, P, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldMap_pvalue(const P<T> &pv, const R &z, const As &... as) {
    return cxx::foldMap(F(), Op(), z, pv, as...);
  }

  static R foldMap_leaf(const leaf<T> &l, const R &z, const As &... as) {
    //     if (cxx::is_async<P<T> >::value)
    //       return cxx::fold(Op(), z, cxx::fmap(foldMap_pvalue, l.values, z,
    //       as...));
    return cxx::foldMap(foldMap_pvalue, Op(), z, l.values, z, as...);
  }

  static R foldMap_ptree(const P<tree<T> > &pt, const R &z, const As &... as) {
    return cxx::foldMap(foldMap_tree_action(), Op(), z, pt, z, as...);
  }

  static R foldMap_branch(const branch<T> &b, const R &z, const As &... as) {
    //     if (cxx::is_async<P<T> >::value)
    //       return cxx::fold(Op(), z, cxx::fmap(foldMap_ptree, b.trees, z,
    //       as...));
    return cxx::foldMap(foldMap_ptree, Op(), z, b.trees, z, as...);
  }

  static R foldMap_tree(const tree<T> &t, const R &z, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_tree);
    return cxx::gfoldl(foldMap_leaf, foldMap_branch, t.node, z, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_tree);

  static R foldMap(F, Op, const R &z, const tree<T> &t, const As &... as) {
    return foldMap_tree(t, z, as...);
  }
};
// Define action exports
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_foldable<F, Op, true, R, T, C, P,
                       As...>::foldMap_tree_evaluate_export_t
    tree_foldable<F, Op, true, R, T, C, P,
                  As...>::foldMap_tree_evaluate_export =
        foldMap_tree_evaluate_export_init();
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P,
          typename... As>
typename tree_foldable<F, Op, true, R, T, C, P,
                       As...>::foldMap_tree_finish_export_t
    tree_foldable<F, Op, true, R, T, C, P, As...>::foldMap_tree_finish_export =
        foldMap_tree_finish_export_init();

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
struct tree_foldable2<F, Op, false, R, T, C, P, T2, As...> {
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldMap2_pvalue(const R &z, const P<T> &pv, const P<T2> &pv2,
                           const F &f, const Op &op, const As &... as) {
    return cxx::foldMap2(f, op, z, pv, pv2, as...);
  }

  static R foldMap2_leaf(const leaf<T> &l, const leaf<T2> &l2, const R &z,
                         const F &f, const Op &op, const As &... as) {
    return cxx::foldMap2(foldMap2_pvalue, z, l.values, l2.values, f, op, as...);
  }

  static R foldMap2_ptree(const R &z, const P<tree<T> > &pt,
                          const P<tree<T2> > &pt2, const F &f, const Op &op,
                          const As &... as) {
    return cxx::foldMap2(foldMap2_tree, z, pt, pt2, f, op, as...);
  }

  static R foldMap2_branch(const branch<T> &b, const branch<T2> &b2, const R &z,
                           const F &f, const Op &op, const As &... as) {
    return cxx::foldMap2(foldMap2_ptree, z, b.trees, b2.trees, f, op, as...);
  }

  static R foldMap2_tree(const R &z, const tree<T> &t, const tree<T2> &t2,
                         const F &f, const Op &op, const As &... as) {
    return cxx::gfoldl2(foldMap2_leaf, foldMap2_branch, t.node, t2.node, z, f,
                        op, as...);
  }

  static R foldMap2(const F &f, const Op &op, const R &z, const tree<T> &t,
                    const tree<T2> &t2, const As &... as) {
    return foldMap2_tree(z, t, t2, f, op, as...);
  }
};

template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
struct tree_foldable2<F, Op, true, R, T, C, P, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static R foldMap2_pvalue(const R &z, const P<T> &pv, const P<T2> &pv2,
                           const As &... as) {
    return cxx::foldMap2(F(), Op(), z, pv, pv2, as...);
  }

  static R foldMap2_leaf(const leaf<T> &l, const leaf<T2> &l2, const R &z,
                         const As &... as) {
    return cxx::foldMap2(foldMap2_pvalue, z, l.values, l2.values, as...);
  }

  static R foldMap2_ptree(const R &z, const P<tree<T> > &pt,
                          const P<tree<T2> > &pt2, const As &... as) {
    return cxx::foldMap2(foldMap2_tree_action(), z, pt, pt2, as...);
  }

  static R foldMap2_branch(const branch<T> &b, const branch<T2> &b2, const R &z,
                           const As &... as) {
    return cxx::foldMap2(foldMap2_ptree, z, b.trees, b2.trees, as...);
  }

  static R foldMap2_tree(const R &z, const tree<T> &t, const tree<T2> &t2,
                         const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap2_tree);
    return cxx::gfoldl2(foldMap2_leaf, foldMap2_branch, t.node, t2.node, z,
                        as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap2_tree);

  static R foldMap2(F, Op, const R &z, const tree<T> &t, const tree<T2> &t2,
                    const As &... as) {
    return foldMap2_tree(z, t, as...);
  }
};
// Define action exports
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
typename tree_foldable2<F, Op, true, R, T, C, P, T2,
                        As...>::foldMap2_tree_evaluate_export_t
    tree_foldable2<F, Op, true, R, T, C, P, T2,
                   As...>::foldMap2_tree_evaluate_export =
        foldMap2_tree_evaluate_export_init();
template <typename F, typename Op, typename R, typename T,
          template <typename> class C, template <typename> class P, typename T2,
          typename... As>
typename tree_foldable2<F, Op, true, R, T, C, P, T2,
                        As...>::foldMap2_tree_finish_export_t
    tree_foldable2<F, Op, true, R, T, C, P, T2,
                   As...>::foldMap2_tree_finish_export =
        foldMap2_tree_finish_export_init();

// TODO: use same identity as in rpc_client
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
R fold(const Op &op, const R &z, const tree<R, C, P> &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap(tree_fold<rpc::is_action<Op>::value, R>::get_identity(), op, z,
                 xs);
}

// iota

template <typename F, template <typename> class tree, typename... As>
struct tree_iota<F, false, tree, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, std::ptrdiff_t, As...> R;

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

  typedef cxx::invoke_of_t<F, std::ptrdiff_t, As...> R;

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

template <typename F, std::ptrdiff_t D, template <typename> class tree,
          typename... As>
struct tree_iota_grid<F, D, false, tree, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  typedef cxx::invoke_of_t<F, grid_region<D>, index<D>, As...> R;

  template <typename U> using leaf = typename tree<U>::leaf_t;
  template <typename U> using branch = typename tree<U>::branch_t;

  template <typename U> using C = typename tree<U>::template container<U>;
  template <typename U> using P = typename tree<U>::template pointer<U>;

  static constexpr std::ptrdiff_t approximate_max_node_size = 10;
  static_assert(approximate_max_node_size > 1, "");
  static constexpr std::ptrdiff_t max_linear_node_size =
      iroot(D, approximate_max_node_size);
  static_assert(max_linear_node_size > 1, "");
  static constexpr std::ptrdiff_t max_node_size = ipow(max_linear_node_size, D);
  static_assert(max_node_size > 1, "");

  static P<R> iota_pvalue(const grid_region<D> &global_range, const index<D> &i,
                          const F &f, const grid_region<D> &range,
                          const As &... as) {
    grid_region<D> range1(i, i + range.istep(), range.istep());
    assert(range1.size() == 1);
    return cxx::iota<P>(f, global_range, range1, as...);
  }

  static leaf<R> iota_leaf(const F &f, const grid_region<D> &global_range,
                           const grid_region<D> &range, std::ptrdiff_t level,
                           const As &... as) {
    assert(level == 0);
    assert(!range.empty() && range.size() <= max_node_size);
    return leaf<R>(
        cxx::iota<C>(iota_pvalue, global_range, range, f, range, as...));
  }

  static tree<R> iota_tree(const grid_region<D> &global_range,
                           const index<D> &i, const F &f,
                           const grid_region<D> &coarse_range,
                           const grid_region<D> &range, std::ptrdiff_t level,
                           const As &... as) {
    grid_region<D> fine_range(i, min(range.imax(), i + coarse_range.istep()),
                              range.istep());
    return iota_level(f, global_range, fine_range, level, as...);
  }

  static P<tree<R> > iota_ptree(const grid_region<D> &global_range, index<D> i,
                                const F &f, const grid_region<D> &coarse_range,
                                const grid_region<D> &range,
                                std::ptrdiff_t level, const As &... as) {
    grid_region<D> coarse_range1(i, i + coarse_range.istep(),
                                 coarse_range.istep());
    assert(coarse_range1.size() == 1);
    return cxx::iota<P>(iota_tree, global_range, coarse_range1, f, coarse_range,
                        range, level, as...);
  }

  static branch<R> iota_branch(const F &f, const grid_region<D> &global_range,
                               const grid_region<D> &range,
                               std::ptrdiff_t level, const As &... as) {
    assert(level >= 0);
    std::ptrdiff_t fact = ipow(max_linear_node_size, level + 1);
    index<D> coarse_step = range.istep() * fact;
    grid_region<D> coarse_range(
        range.imin(), align_ceil(range.imax(), coarse_step), coarse_step);
    assert(coarse_range.size() >= 1 && coarse_range.size() <= max_node_size);
    return branch<R>(cxx::iota<C>(iota_ptree, global_range, coarse_range, f,
                                  coarse_range, range, level, as...));
  }

  static tree<R> iota_level(const F &f, const grid_region<D> &global_range,
                            const grid_region<D> &range, std::ptrdiff_t level,
                            const As &... as) {
    if (level == 0)
      return tree<R>(iota_leaf(f, global_range, range, level, as...));
    return tree<R>(iota_branch(f, global_range, range, level - 1, as...));
  }

  static tree<R> iota(const F &f, const grid_region<D> &global_range,
                      const grid_region<D> &range, const As &... as) {
    if (range.empty())
      return tree<R>();
    std::ptrdiff_t levels =
        range.size() == 1 ? 0 : ilog(max_linear_node_size,
                                     maxval(range.shape()) - 1);
    assert(ipow(max_node_size, levels + 1) > range.size());
    return iota_level(f, global_range, range, levels, as...);
  }
};

template <typename F, std::ptrdiff_t D, template <typename> class tree,
          typename... As>
struct tree_iota_grid<F, D, true, tree, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, grid_region<D>, index<D>, As...> R;

  template <typename U> using leaf = typename tree<U>::leaf_t;
  template <typename U> using branch = typename tree<U>::branch_t;

  template <typename U> using C = typename tree<U>::template container<U>;
  template <typename U> using P = typename tree<U>::template pointer<U>;

  static constexpr std::ptrdiff_t approximate_max_node_size = 10;
  static_assert(approximate_max_node_size > 1, "");
  static constexpr std::ptrdiff_t max_linear_node_size =
      iroot(D, approximate_max_node_size);
  static_assert(max_linear_node_size > 1, "");
  static constexpr std::ptrdiff_t max_node_size = ipow(max_linear_node_size, D);
  static_assert(max_node_size > 1, "");

  static P<R> iota_pvalue(const grid_region<D> &global_range, const index<D> &i,
                          const F &f, const grid_region<D> &range,
                          const As &... as) {
    grid_region<D> range1(i, i + range.istep(), range.istep());
    assert(range1.size() == 1);
    return cxx::iota<P>(f, global_range, range1, as...);
  }

  static leaf<R> iota_leaf(const F &f, const grid_region<D> &global_range,
                           const grid_region<D> &range, std::ptrdiff_t level,
                           const As &... as) {
    assert(level == 0);
    assert(!range.empty() && range.size() <= max_node_size);
    return leaf<R>(
        cxx::iota<C>(iota_pvalue, global_range, range, f, range, as...));
  }

  static tree<R> iota_tree(const grid_region<D> &global_range,
                           const index<D> &i, const F &f,
                           const grid_region<D> &coarse_range,
                           const grid_region<D> &range, std::ptrdiff_t level,
                           const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);
    grid_region<D> fine_range(i, min(range.imax(), i + coarse_range.istep()),
                              range.istep());
    return iota_level(f, global_range, fine_range, level, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(iota_tree);

  static P<tree<R> > iota_ptree(const grid_region<D> &global_range, index<D> i,
                                const F &f, const grid_region<D> &coarse_range,
                                const grid_region<D> &range,
                                std::ptrdiff_t level, const As &... as) {
    grid_region<D> coarse_range1(i, i + coarse_range.istep(),
                                 coarse_range.istep());
    assert(coarse_range1.size() == 1);
    return cxx::iota<P>(iota_tree_action(), global_range, coarse_range1, f,
                        coarse_range, range, level, as...);
  }

  static branch<R> iota_branch(const F &f, const grid_region<D> &global_range,
                               const grid_region<D> &range,
                               std::ptrdiff_t level, const As &... as) {
    assert(level >= 0);
    std::ptrdiff_t fact = ipow(max_linear_node_size, level + 1);
    index<D> coarse_step = range.istep() * fact;
    grid_region<D> coarse_range(
        range.imin(), align_ceil(range.imax(), coarse_step), coarse_step);
    assert(coarse_range.size() >= 1 && coarse_range.size() <= max_node_size);
    return branch<R>(cxx::iota<C>(iota_ptree, global_range, coarse_range, f,
                                  coarse_range, range, level, as...));
  }

  static tree<R> iota_level(const F &f, const grid_region<D> &global_range,
                            const grid_region<D> &range, std::ptrdiff_t level,
                            const As &... as) {
    if (level == 0)
      return tree<R>(iota_leaf(f, global_range, range, level, as...));
    return tree<R>(iota_branch(f, global_range, range, level - 1, as...));
  }

  static tree<R> iota(const F &f, const grid_region<D> &global_range,
                      const grid_region<D> &range, const As &... as) {
    if (range.empty())
      return tree<R>();
    std::ptrdiff_t levels =
        range.size() == 1 ? 0 : ilog(max_linear_node_size,
                                     maxval(range.shape()) - 1);
    assert(ipow(max_node_size, levels + 1) > range.size());
    return iota_level(f, global_range, range, levels, as...);
  }
};
// Define action exports
template <typename F, std::ptrdiff_t D, template <typename> class tree,
          typename... As>
typename tree_iota_grid<F, D, true, tree, As...>::iota_tree_evaluate_export_t
    tree_iota_grid<F, D, true, tree, As...>::iota_tree_evaluate_export =
        iota_tree_evaluate_export_init();
template <typename F, std::ptrdiff_t D, template <typename> class tree,
          typename... As>
typename tree_iota_grid<F, D, true, tree, As...>::iota_tree_finish_export_t
    tree_iota_grid<F, D, true, tree, As...>::iota_tree_finish_export =
        iota_tree_finish_export_init();

// functor

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_functor<F, false, T, C, P, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, As...> R;

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

  typedef cxx::invoke_of_t<F, T, As...> R;

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

  typedef cxx::invoke_of_t<F, T, T2, As...> R;

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

  typedef cxx::invoke_of_t<F, T, T2, As...> R;

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
  static_assert(std::is_same<cxx::invoke_of_t<G, T, bool>, B>::value, "");

  typedef cxx::invoke_of_t<F, T, B, B> R;

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
    } else if (s > 1) {
      values[0] =
          cxx::fmap3(F(), l.values[0], bm, cxx::fmap(G(), l.values[1], false));
#pragma omp simd
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
    } else if (s > 1) {
      trees[0] =
          cxx::fmap(stencil_fmap_tree_action(), b.trees[0], bm,
                    cxx::mbind(b.trees[1], get_boundary_tree_action(), false));
#pragma omp simd
      for (std::size_t i = 1; i < s - 1; ++i) {
        trees[i] = cxx::fmap(
            stencil_fmap_tree_action(), b.trees[i],
            cxx::mbind(b.trees[i - 1], get_boundary_tree_action(), true),
            cxx::mbind(b.trees[i + 1], get_boundary_tree_action(), false));
      }
      trees[s - 1] = cxx::fmap(
          stencil_fmap_tree_action(), b.trees[s - 1],
          cxx::mbind(b.trees[s - 2], get_boundary_tree_action(), true), bp);
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
    return cxx::mbind(!face_upper ? b.trees.front() : b.trees.back(),
                      get_boundary_tree_action(), face_upper);
  }

  static P<B> get_boundary_tree(const tree<T> &t, bool face_upper) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
    return cxx::gfoldl(get_boundary_leaf, get_boundary_branch, t.node,
                       face_upper);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);

  // static P<B> get_boundary(G, const tree<T> &xs, bool face_upper) {
  //   return get_boundary_tree(xs, face_upper);
  // }
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

template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
struct tree_boundary {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static leaf<R> boundary_leaf(const leaf<T> &l, std::ptrdiff_t dir, bool face,
                               const As &... as) {
    return leaf<R>(cxx::boundary(
        [](const P<T> &pv, std::ptrdiff_t dir, bool face,
           const As &... as) { return fmap(F(), pv, dir, face, as...); },
        l.values, dir, face, as...));
  }

  static branch<R> boundary_branch(const branch<T> &b, std::ptrdiff_t dir,
                                   bool face, const As &... as) {
    return branch<R>(cxx::boundary([](const P<tree<T> > &pt, std::ptrdiff_t dir,
                                      bool face, const As &... as) {
                                     return cxx::fmap(boundary_tree_action(),
                                                      pt, dir, face, as...);
                                   },
                                   b.trees, dir, face, as...));
  }

  static tree<R> boundary_tree(const tree<T> &t, std::ptrdiff_t dir, bool face,
                               const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(boundary_tree);
    return tree<R>(
        cxx::gmap(boundary_leaf, boundary_branch, t.node, dir, face, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(boundary_tree);

  static tree<R> boundary(F, const tree<T> &xs, std::ptrdiff_t dir, bool face,
                          const As &... as) {
    return boundary_tree(xs, dir, face, as...);
  }
};
// Define action exports
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_boundary<F, T, C, P, As...>::boundary_tree_evaluate_export_t
    tree_boundary<F, T, C, P, As...>::boundary_tree_evaluate_export =
        boundary_tree_evaluate_export_init();
template <typename F, typename T, template <typename> class C,
          template <typename> class P, typename... As>
typename tree_boundary<F, T, C, P, As...>::boundary_tree_finish_export_t
    tree_boundary<F, T, C, P, As...>::boundary_tree_finish_export =
        boundary_tree_finish_export_init();

template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
struct tree_stencil_functor_boundaries<F, G, true, T, C, P, B, D> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<G>::value, "");
  static_assert(
      std::is_same<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>, B>::value, "");

  typedef cxx::invoke_of_t<F, T, cxx::boundaries<B, D> > R;

  template <typename U> using leaf = leaf<U, C, P>;
  template <typename U> using branch = branch<U, C, P>;
  template <typename U> using tree = tree<U, C, P>;

  static leaf<R> stencil_fmap_leaf(const leaf<T> &l,
                                   const cxx::boundaries<leaf<B>, D> &bs) {
    // std::cerr << "stencil_fmap_leaf l=" << l << " bs=" << bs << "\n";
    return leaf<R>(cxx::stencil_fmap(
        [](const P<T> &pv, const boundaries<P<B>, D> &pbs) {
          return cxx::fmap_boundaries(F(), pv, pbs);
        },
        [](const P<T> &pv, std::ptrdiff_t dir,
           bool face) { return cxx::fmap(G(), pv, dir, face); },
        l.values, fmap([](const auto &b) { return b.values; }, bs)));
  }

  static branch<R>
  stencil_fmap_branch(const branch<T> &b,
                      const cxx::boundaries<branch<B>, D> &bs) {
    // std::cerr << "stencil_fmap_branch b=" << b << " bs=" << bs << "\n";
    return branch<R>(cxx::stencil_fmap(
        [](const P<tree<T> > &pt, const boundaries<P<tree<B> >, D> &pbs) {
          return cxx::fmap_boundaries(stencil_fmap_tree_action(), pt, pbs);
        },
        [](const P<tree<T> > &pt, std::ptrdiff_t dir, bool face) {
          return cxx::fmap(get_boundary_tree_action(), pt, dir, face);
        },
        b.trees, fmap([](const auto &b) { return b.trees; }, bs)));
  }

  static tree<R> stencil_fmap_tree(const tree<T> &t,
                                   const cxx::boundaries<tree<B>, D> &bs) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);
    // std::cerr << "stencil_fmap_tree t=" << t << " bs=" << bs << "\n";
    return tree<R>(
        cxx::gmap_boundaries(stencil_fmap_leaf, stencil_fmap_branch, t.node,
                             fmap([](const auto &bt) { return bt.node; }, bs)));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(stencil_fmap_tree);

  static tree<R> stencil_fmap(F, G, const tree<T> &xs,
                              const cxx::boundaries<tree<B>, D> &bss) {
    return stencil_fmap_tree(xs, bss);
  }

  // TODO: use tree_boundary's routines instead of the ones below
  static leaf<B> get_boundary_leaf(const leaf<T> &l, std::ptrdiff_t dir,
                                   bool face) {
    // std::cerr << "get_boundary_leaf l=" << l << " dir==" << dir
    //           << " face=" << face << "\n";
    return leaf<B>(boundary([](const P<T> &pv, std::ptrdiff_t dir,
                               bool face) { return fmap(G(), pv, dir, face); },
                            l.values, dir, face));
  }

  static branch<B> get_boundary_branch(const branch<T> &b, std::ptrdiff_t dir,
                                       bool face) {
    // std::cerr << "get_boundary_branch b=" << b << " dir==" << dir
    //           << " face=" << face << "\n";
    return branch<B>(
        boundary([](const P<tree<T> > &pt, std::ptrdiff_t dir, bool face) {
                   return fmap(get_boundary_tree_action(), pt, dir, face);
                 },
                 b.trees, dir, face));
  }

  static tree<B> get_boundary_tree(const tree<T> &t, std::ptrdiff_t dir,
                                   bool face) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
    // std::cerr << "get_boundary_tree t=" << t << " dir==" << dir
    //           << " face=" << face << "\n";
    return tree<B>(
        cxx::gmap(get_boundary_leaf, get_boundary_branch, t.node, dir, face));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(get_boundary_tree);
};
// Define action exports
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
typename tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                         D>::stencil_fmap_tree_evaluate_export_t
    tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                    D>::stencil_fmap_tree_evaluate_export =
        stencil_fmap_tree_evaluate_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
typename tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                         D>::stencil_fmap_tree_finish_export_t
    tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                    D>::stencil_fmap_tree_finish_export =
        stencil_fmap_tree_finish_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
typename tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                         D>::get_boundary_tree_evaluate_export_t
    tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                    D>::get_boundary_tree_evaluate_export =
        get_boundary_tree_evaluate_export_init();
template <typename F, typename G, typename T, template <typename> class C,
          template <typename> class P, typename B, std::ptrdiff_t D>
typename tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                         D>::get_boundary_tree_finish_export_t
    tree_stencil_functor_boundaries<F, G, true, T, C, P, B,
                                    D>::get_boundary_tree_finish_export =
        get_boundary_tree_finish_export_init();

// monad

template <template <typename> class C, typename T1,
          typename T = std::decay_t<T1> >
std::enable_if_t<cxx::is_tree<C<T> >::value, C<T> > munit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_tree<C<T> >::value, C<T> > mmake(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, template <typename> class C1, template <typename> class P,
          typename CCT = cxx::tree<cxx::tree<T, C1, P>, C1, P>,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const cxx::tree<cxx::tree<T, C1, P>, C1, P> &xss) {
  return xss.mjoin();
}

// TODO: Implement this directly
template <typename T, template <typename> class C, template <typename> class P,
          typename F, typename... As>
auto mbind(const cxx::tree<T, C, P> &xs, const F &f, const As &... as) {
  return mjoin(fmap(f, xs, as...));
}

template <template <typename> class C, typename T>
std::enable_if_t<cxx::is_tree<C<T> >::value, C<T> > mzero() {
  return C<T>();
}

template <typename T, template <typename> class C1, template <typename> class P,
          typename... As, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<T> mplus(const cxx::tree<T, C1, P> &xs, const As &... as) {
  static_assert(cxx::all<std::is_same<As, C<T> >::value...>::value, "");
  return C<T>(typename C<T>::mplus(), xs, as...);
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_tree<C<T> >::value, C<T> > msome(T &&x, As &&... as) {
  static_assert(cxx::all<std::is_same<As, T>::value...>::value, "");
  // TODO: Implement directly
  return mplus(munit<C>(std::forward<T>(x)), munit<C>(std::forward<As>(as))...);
}
}

#define CXX_TREE_HH_DONE
#else
#ifndef CXX_TREE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_TREE_HH
