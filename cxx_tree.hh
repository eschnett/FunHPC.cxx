#ifndef CXX_TREE_HH
#define CXX_TREE_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_either.hh"
#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cxx {

// A tree class, which is a distributed version of a vector

// A tree is either a pointer to a leaf or a pointer to a branch, a
// branch is container of trees, and a leaf is a containers of values.

template <typename T, template <typename> class C, template <typename> class P>
class tree;

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

  C<T> values;

public:
  // leaf() {}
  explicit leaf() { assert(0); } // forbid empty leaf
  explicit leaf(const C<T> &values) : values(values) {}
  explicit leaf(const T &value) : leaf(monad::make<C, T>(value)) {}
  leaf(const leaf &l) : values(l.values) {}

  // size
  bool empty() const { return values.empty(); }
  std::size_t size() const { return values.size(); }

  // fmap
  struct fmap : std::tuple<> {};

private:
  template <typename U> struct unwrap_leaf {
    typedef U type;
    const U &operator()(const U &x) const { return x; }
  };
  template <typename U> struct unwrap_leaf<leaf<U, C, P> > {
    typedef U type;
    const C<U> &operator()(const leaf<U, C, P> &x) const { return x.values; }
  };

public:
  template <typename U, typename... As, typename F,
            typename R = typename cxx::invoke_of<
                F, U, typename unwrap_leaf<As>::type...>::type>
  leaf(typename std::enable_if<std::is_same<R, T>::value, fmap>::type,
       const F &f, const leaf<U, C, P> &xs, const As &... as)
      : values(cxx::fmap(f, xs.values, unwrap_leaf<As>()(as)...)) {}

  // foldl
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    R r(z);
    for (const auto &v : values)
      r = f(r, v);
    return r;
  }
};

template <typename T, template <typename> class C, template <typename> class P>
class branch {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class branch;

  C<tree<T, C, P> > trees;

public:
  explicit branch() {}
  explicit branch(const C<tree<T, C, P> > &trees) : trees(trees) {}
  explicit branch(const tree<T, C, P> &t)
      : branch(monad::make<C, tree<T, C, P> >(t)) {}
  branch(const branch &b) : trees(b.trees) {}

  // size
  bool empty() const {
    // TODO: use foldl
    return std::all_of(trees.begin(), trees.end(),
                       [](const tree<T, C, P> &t) { return t.empty(); });
  }
  std::size_t size() const {
    // TODO: use foldl
    return std::accumulate(
        trees.begin(), trees.end(), 0,
        [](std::size_t s, const tree<T, C, P> &t) { return s + t.size(); });
  }

  // fmap
  struct fmap : std::tuple<> {};

private:
  template <typename U> struct unwrap_branch {
    typedef U type;
    typedef U unwrapped_type;
    const unwrapped_type &operator()(const U &x) const { return x; }
  };
  template <typename U> struct unwrap_branch<branch<U, C, P> > {
    typedef U type;
    typedef tree<U, C, P> unwrapped_type;
    const C<unwrapped_type> &operator()(const branch<U, C, P> &x) const {
      return x.trees;
    }
  };

public:
  template <typename U, typename... As, typename F,
            typename R = typename cxx::invoke_of<
                F, U, typename unwrap_branch<As>::type...>::type>
  branch(typename std::enable_if<std::is_same<R, T>::value, fmap>::type,
         const F &f, const branch<U, C, P> &xs, const As &... as)
      : trees(cxx::fmap(
            [f](const tree<U, C, P> &xs,
                const typename unwrap_branch<As>::unwrapped_type &... as) {
              return tree<T, C, P>(typename tree<T, C, P>::fmap(), f, xs,
                                   as...);
            },
            xs.trees, unwrap_branch<As>()(as)...)) {}

  // join
  struct join : std::tuple<> {};
  branch(join, const leaf<tree<T, C, P>, C, P> &l) : branch(l.values) {}
  branch(join, const branch<tree<T, C, P>, C, P> &b)
      : branch(cxx::fmap([](const tree<tree<T, C, P>, C, P> &t) {
                           return tree<T, C, P>(typename tree<T, C, P>::join(),
                                                t);
                         },
                         b.trees)) {}

  // plus
  struct plus : std::tuple<> {};
  branch(plus, const C<tree<T, C, P> > &t) : branch(t) {}

  // foldl
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    R r(z);
    for (const auto &t : trees)
      r = cxx::invoke(f, r, t.foldl(f, z));
    return r;
  }
};

template <typename T, template <typename> class C, template <typename> class P>
class tree {

  template <typename T1, template <typename> class C1,
            template <typename> class P1>
  friend class tree;

public:
  typedef T value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef const T &const_reference;
  typedef const T *const_pointer;

private:
  typedef cxx::either<P<leaf<T, C, P> >, P<branch<T, C, P> > > node_t;
  node_t node;

public:
  // explicit tree() : node(monad::zero<P, branch<T, C, P> >()) {}
  explicit tree() : node(monad::make<P, branch<T, C, P> >()) {}
  explicit tree(const P<leaf<T, C, P> > &pl) : node(pl) {}
  explicit tree(const P<branch<T, C, P> > &pb) : node(pb) {}
  explicit tree(const T &value) : node(monad::make<P, leaf<T, C, P> >(value)) {}
  tree(const tree &t) : node(t.node) {}

  bool invariant() const {
    return node.gmap([](const P<leaf<T, C, P> > &pl) { return bool(pl); },
                     [](const P<branch<T, C, P> > &pb) { return bool(pb); });
  }

  bool empty() const {
    return node.gfoldl([](const P<leaf<T, C, P> > &pl) { return pl->empty(); },
                       [](const P<branch<T, C, P> > &pb) {
      return pb->empty();
    });
  }
  size_type size() const {
    return node.gfoldl([](const P<leaf<T, C, P> > &pl) { return pl->size(); },
                       [](const P<branch<T, C, P> > &pb) {
      return pb->size();
    });
  }

  // fmap
  struct fmap : std::tuple<> {};

private:
  template <typename U> struct unwrap_tree {
    typedef U type;
    typedef U unwrapped_type_leaf;
    typedef U unwrapped_type_branch;
    const U &operator()(const U &x) const { return x; }
  };
  template <typename U> struct unwrap_tree<tree<U, C, P> > {
    typedef U type;
    typedef leaf<U, C, P> unwrapped_type_leaf;
    typedef branch<U, C, P> unwrapped_type_branch;
    const cxx::either<unwrapped_type_leaf, unwrapped_type_branch> &
    operator()(const tree<U, C, P> &x) const {
      return x.node;
    }
  };

public:
  template <typename U, typename... As, typename F,
            typename R = typename cxx::invoke_of<
                F, U, typename unwrap_tree<As>::type...>::type>
  tree(typename std::enable_if<std::is_same<R, T>::value, fmap>::type,
       const F &f, const tree<U, C, P> &xs, const As &... as)
      : node(
            typename node_t::gmap(),
            [f](const P<leaf<U, C, P> > &xs,
                P<typename unwrap_tree<As>::unwrapped_type_leaf> &... as) {
              return cxx::fmap(
                  [f](const leaf<U, C, P> &xs,
                      typename unwrap_tree<As>::unwrapped_type_leaf &... as) {
                    return leaf<T, C, P>(typename leaf<T, C, P>::fmap(), f, xs,
                                         as...);
                  },
                  xs, as...);
            },
            [f](const P<branch<U, C, P> > &xs,
                P<typename unwrap_tree<As>::unwrapped_type_branch> &... as) {
              return cxx::fmap(
                  [f](const branch<U, C, P> &xs,
                      typename unwrap_tree<As>::unwrapped_type_branch &... as) {
                    return branch<T, C, P>(typename branch<T, C, P>::fmap(), f,
                                           xs, as...);
                  },
                  xs, as...);
            },
            xs.node, unwrap_tree<As>()(as)...) {}

  // join
  struct join : std::tuple<> {};
  tree(join, const tree<tree<T, C, P>, C, P> &t)
      : node(t.node.is_left()
                 ? cxx::fmap([](const leaf<tree<T, C, P>, C, P> &l) {
                               return branch<T, C, P>(
                                   typename branch<T, C, P>::join(), l);
                             },
                             t.node.left())
                 : cxx::fmap([](const branch<tree<T, C, P>, C, P> &b) {
                               return branch<T, C, P>(
                                   typename branch<T, C, P>::join(), b);
                             },
                             t.node.right())) {}

  // plus
  struct plus : std::tuple<> {};
  tree(plus, const tree &x, const tree &y)
      : node(monad::make<P, branch<T, C, P> >(typename branch<T, C, P>::plus(),
                                              monad::some<C>(x, y))) {}

  // foldl
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    return node.gfoldl([f, z](const P<leaf<T, C, P> > &pl) {
                         return pl->foldl(f, z);
                       },
                       [f, z](const P<branch<T, C, P> > &pb) {
      return pb->foldl(f, z);
    });
  }

  // size
  bool empty_slow() const {
    return foldl([](bool x, const T &v) { return false; }, true);
  }
  size_type size_slow() const {
    return foldl([](size_type s, const T &v) { return s + 1; }, size_type(0));
  }
};

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename T, template <typename> class C, template <typename> class P>
struct kinds<tree<T, C, P> > {
  typedef T type;
  template <typename U> using constructor = tree<U, C, P>;
};

// foldable
template <typename R, typename T, template <typename> class C,
          template <typename> class P, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const tree<T, C, P> &x) {
  return x.foldl(f, z);
}

// functor
namespace detail {
template <typename T> struct unwrap_tree {
  typedef T type;
};
template <typename T, template <typename> class C, template <typename> class P>
struct unwrap_tree<tree<T, C, P> > {
  typedef T type;
};
}
template <typename T, template <typename> class C1, template <typename> class P,
          typename... As, typename F, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_tree<As>::type...>::type>
C<R> fmap(const F &f, const cxx::tree<T, C1, P> &xs, const As &... as) {
  return C<R>(typename C<R>::fmap(), f, xs, as...);
}

namespace monad {

namespace detail {
template <typename T> struct is_cxx_tree : std::false_type {};
template <typename T, template <typename> class C, template <typename> class P>
struct is_cxx_tree<tree<T, C, P> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_cxx_tree<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return M<T>(std::forward<T>(x));
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_cxx_tree<M<T> >::value, M<T> >::type
make(As &&... as) {
  return M<T>(T(std::forward<As>(as)...));
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_tree<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  return M<T>(typename M<T>::join(), x);
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_cxx_tree<M<T> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  return join<M>(fmap(f, x));
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_tree<M<T> >::value, M<T> >::type zero() {
  return M<T>();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_tree<M<T> >::value, M<T> >::type
plus(const M<T> &x, const M<T> &y) {
  return M<T>(typename M<T>::plus(), x, y);
}
}
}

#endif // #ifndef CXX_TREE_HH
