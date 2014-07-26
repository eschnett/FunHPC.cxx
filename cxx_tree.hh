#ifndef CXX_TREE_HH
#define CXX_TREE_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_either.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

#include <cereal/access.hpp>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <tuple>
#include <type_traits>
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

  C<P<T> > values;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(values); }

public:
  explicit leaf() {}
  explicit leaf(const C<P<T> > &vs) : values(vs) {}
  explicit leaf(const P<T> &v) : leaf(cxx::unit<C>(v)) {}
  leaf(const leaf &l) : values(l.values) {}
  leaf(leaf &&l) : values(std::move(l.values)) {}

  // size
  bool empty() const { return values.empty(); }
  std::size_t size() const { return values.size(); }

  // foldable
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    return cxx::foldl([f](const R &z,
                          const P<T> &pv) { return cxx::foldl(f, z, pv); },
                      z, values);
  }

  // functor
  struct fmap : std::tuple<> {};

private:
  template <typename U> struct unwrap_leaf {
    typedef U type;
    typedef U pointer_type;
    const U &operator()(const U &x) const { return x; }
  };
  template <typename U> struct unwrap_leaf<leaf<U, C, P> > {
    typedef U type;
    typedef P<U> pointer_type;
    const C<P<U> > &operator()(const leaf<U, C, P> &x) const {
      return x.values;
    }
  };

public:
  template <typename U, typename... As, typename F,
            typename R = typename cxx::invoke_of<
                F, U, typename unwrap_leaf<As>::type...>::type>
  leaf(typename std::enable_if<std::is_same<R, T>::value, fmap>::type,
       const F &f, const leaf<U, C, P> &xs, const As &... as)
      : values(cxx::fmap(
            [f](const P<U> &pv,
                const typename unwrap_leaf<As>::pointer_type &... as) {
              return cxx::fmap(f, pv, as...);
            },
            xs.values, unwrap_leaf<As>()(as)...)) {}

  struct stencil_fmap : std::tuple<> {};
  template <typename U, typename B, typename F, typename G>
  leaf(
      typename std::enable_if<
          ((std::is_same<typename cxx::invoke_of<F, U, B, B>::type,
                         T>::value) &&
           (std::is_same<typename cxx::invoke_of<G, U, bool>::type, B>::value)),
          stencil_fmap>::type,
      const F &f, const G &g, const cxx::leaf<U, C, P> &l, const B &bm,
      const B &bp) {
    const C<P<U> > &xs = l.values;
    size_t s = xs.size();
    assert(s >= 2); // TODO: This can be avoided
    C<P<T> > rs(s);
    // TODO: use array<...,N>, and allow multiple ghost zones
    // TODO: add push_back equivalent to constructor
    // TODO: for efficiency: don't use push_back, use something else
    // that requires preallocation, that doesn't reallocate, and which
    // turns into an indexed loop when used for vectors
    rs[0] = cxx::fmap(f, xs[0], bm, cxx::fmap(g, xs[1], false));
    for (size_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::fmap(f, xs[i], cxx::fmap(g, xs[i - 1], true),
                        cxx::fmap(g, xs[i + 1], false));
    rs[s - 1] = cxx::fmap(f, xs[s - 1], cxx::fmap(g, xs[s - 2], true), bp);
    values = std::move(rs);
  }

  template <typename G, typename B = typename cxx::invoke_of<G, T, bool>::type>
  B get_boundary(const G &g, bool face_upper) const {
    assert(!values.empty());
    // TODO: use foldl1
    return cxx::foldl([g, face_upper](const B &, const T &x) {
                        return cxx::invoke(g, x, face_upper);
                      },
                      B(), !face_upper ? values.front() : values.back());
  }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::element_type>
  typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
                          branch<R, C, P> >::type
  join() const {
    return branch<R, C, P>(values);
  }

  // iota
  struct iota : std::tuple<> {};
  static constexpr std::ptrdiff_t default_size() {
    return tree<T, C, P>::default_size();
  }
  template <typename... As, typename F, typename U = T>
  leaf(typename std::enable_if<
           std::is_same<U, typename cxx::invoke_of<F, std::ptrdiff_t,
                                                   As...>::type>::value,
           iota>::type,
       const F &f, std::ptrdiff_t imin, std::ptrdiff_t imax,
       std::ptrdiff_t istep, const As &... as)
      : values(cxx::iota<C>([f](std::ptrdiff_t i, const As &... as) {
                              return cxx::unit<P>(cxx::invoke(f, i, as...));
                            },
                            imin, imax, istep, as...)) {}
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

  typedef T element_type;

  C<P<tree<T, C, P> > > trees;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(trees); }

public:
  explicit branch() {}
  explicit branch(const C<P<tree<T, C, P> > > &ts) : trees(ts) {}
  explicit branch(const P<tree<T, C, P> > &t) : branch(cxx::unit<C>(t)) {}
  branch(const branch &b) : trees(b.trees) {}
  branch(branch &&b) : trees(std::move(b.trees)) {}

  // size
  bool empty() const {
    return cxx::foldl([](bool is_empty, const P<tree<T, C, P> > &t) {
                        return cxx::foldl([](bool is_empty,
                                             const tree<T, C, P> &t) {
                                            return is_empty && t.empty();
                                          },
                                          true, t);
                      },
                      true, trees);
  }
  std::size_t size() const {
    return cxx::foldl([](std::size_t size, const P<tree<T, C, P> > &t) {
                        return cxx::foldl([](std::size_t size,
                                             const tree<T, C, P> &t) {
                                            return size + t.size();
                                          },
                                          std::size_t(0), t);
                      },
                      std::size_t(0), trees);
  }

  // foldable
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    return cxx::foldl([f](const R &z, const P<tree<T, C, P> > &pt) {
                        return cxx::foldl([f](const R &z,
                                              const tree<T, C, P> t) {
                                            // return cxx::foldl(f, z, t);
                                            return t.foldl(f, z);
                                          },
                                          z, pt);
                      },
                      z, trees);
  }

  // functor
  struct fmap : std::tuple<> {};

private:
  template <typename U> struct unwrap_branch {
    typedef U type;
    typedef U pointer_type;
    const U &operator()(const U &x) const { return x; }
  };
  template <typename U> struct unwrap_branch<branch<U, C, P> > {
    typedef U type;
    typedef P<tree<U, C, P> > pointer_type;
    const C<P<tree<U, C, P> > > &operator()(const branch<U, C, P> &x) const {
      return x.trees;
    }
  };

public:
  template <typename U, typename... As, typename F,
            typename R = typename cxx::invoke_of<
                F, U, typename unwrap_branch<As>::type...>::type>
  branch(typename std::enable_if<std::is_same<R, T>::value, fmap>::type,
         const F &f, const branch<U, C, P> &xs, const As &... as)
      : branch(cxx::fmap(
            [f](const P<tree<U, C, P> > &pt,
                const typename unwrap_branch<As>::pointer_type &... as) {
              return cxx::fmap(
                  [f](const tree<U, C, P> &t,
                      const typename unwrap_branch<As>::type &... as) {
                    return tree<T, C, P>(typename tree<T, C, P>::fmap(), f, t,
                                         as...);
                  },
                  pt, as...);
            },
            xs.trees, unwrap_branch<As>()(as)...)) {}

  struct stencil_fmap : std::tuple<> {};
  template <typename U, typename B, typename F, typename G>
  branch(
      typename std::enable_if<
          ((std::is_same<typename cxx::invoke_of<F, U, B, B>::type,
                         T>::value) &&
           (std::is_same<typename cxx::invoke_of<G, U, bool>::type, B>::value)),
          stencil_fmap>::type,
      const F &f, const G &g, const cxx::branch<U, C, P> &b, const B &bm,
      const B &bp) {
    const C<P<tree<U, C, P> > > &xs = b.trees;
    size_t s = xs.size();
    assert(s >= 2);
    C<P<tree<T, C, P> > > rs(s);
    auto recurse = [f, g](const tree<U, C, P> &t, const B &bm, const B &bp) {
      return tree<T, C, P>(typename tree<T, C, P>::stencil_fmap(), f, g, t, bm,
                           bp);
    };
    rs[0] = cxx::fmap(
        recurse, xs[0], bm,
        cxx::fmap(&tree<T, C, P>::template get_boundary<G>, xs[1], g, false));
    for (size_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::fmap(recurse, xs[i],
                        cxx::fmap(&tree<T, C, P>::template get_boundary<G>,
                                  xs[i - 1], g, true),
                        cxx::fmap(&tree<T, C, P>::template get_boundary<G>,
                                  xs[i + 1], g, false));
    rs[s - 1] = cxx::fmap(
        recurse, xs[s - 1],
        cxx::fmap(&tree<T, C, P>::template get_boundary<G>, xs[s - 2], g, true),
        bp);
    trees = std::move(rs);
  }

  template <typename G, typename B = typename cxx::invoke_of<G, T, bool>::type>
  B get_boundary(const G &g, bool face_upper) const {
    assert(!trees.empty());
    // TODO: use foldl1
    return cxx::foldl([g, face_upper](const B &, const tree<T, C, P> &t) {
                        return t.get_boundary(g, face_upper);
                      },
                      B(), !face_upper ? trees.front() : trees.back());
  }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::element_type>
  typename std::enable_if<std::is_same<T, tree<R, C, P> >::value,
                          branch<R, C, P> >::type
  join() const {
    return branch<R, C, P>(
        cxx::fmap([](const P<tree<T, C, P> > &pt) {
                    return cxx::fmap([](const tree<T, C, P> &t) {
                                       return t.join();
                                     },
                                     pt);
                  },
                  trees));
  }

  // iota
  struct iota : std::tuple<> {};
  static constexpr std::ptrdiff_t default_size() {
    return tree<T, C, P>::default_size();
  }
  static constexpr std::ptrdiff_t
  branch_istep(std::ptrdiff_t imin, std::ptrdiff_t imax, std::ptrdiff_t istep) {
    return cxx::align_ceil(cxx::div_ceil(imax - imin, default_size()), istep);
  }
  template <typename... As, typename F, typename U = T>
  branch(typename std::enable_if<
             std::is_same<U, typename cxx::invoke_of<F, std::ptrdiff_t,
                                                     As...>::type>::value,
             iota>::type,
         const F &f, std::ptrdiff_t imin, std::ptrdiff_t imax,
         std::ptrdiff_t istep, const As &... as)
      : trees(cxx::iota<C>([f, imin, imax, istep](std::ptrdiff_t i,
                                                  const As &... as) {
                             auto istep1 = branch_istep(imin, imax, istep);
                             return cxx::unit<P>(tree<T, C, P>(
                                 typename tree<T, C, P>::iota(), f, i,
                                 std::min(i + istep1, imax), istep, as...));
                           },
                           imin, imax, branch_istep(imin, imax, istep),
                           as...)) {}
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

public:
  typedef T value_type;
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
  explicit tree(const branch<T, C, P> &b) : node(b) {}
  explicit tree(const P<T> &pv) : tree(leaf<T, C, P>(pv)) {}
  explicit tree(const T &v) : tree(cxx::unit<P>(v)) {}
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

  // foldable
  template <typename R, typename F>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
  foldl(const F &f, const R &z) const {
    return node.gfoldl([f, z](const leaf<T, C, P> &l) { return l.foldl(f, z); },
                       [f, z](const branch<T, C, P> &b) {
      return b.foldl(f, z);
    });
  }

  // functor
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
    const cxx::either<leaf<U, C, P>, branch<U, C, P> > &
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
            [f](const leaf<U, C, P> &l,
                const typename unwrap_tree<As>::unwrapped_type_leaf &... as) {
              return leaf<T, C, P>(typename leaf<T, C, P>::fmap(), f, l, as...);
            },
            [f](const branch<U, C, P> &b,
                const typename unwrap_tree<As>::unwrapped_type_branch &... as) {
              return branch<T, C, P>(typename branch<T, C, P>::fmap(), f, b,
                                     as...);
            },
            xs.node, unwrap_tree<As>()(as)...) {}

  struct stencil_fmap : std::tuple<> {};
  template <typename U, typename B, typename F, typename G,
            typename R = typename cxx::invoke_of<F, U, B, B>::type>
  tree(typename std::enable_if<
           ((std::is_same<typename cxx::invoke_of<G, U, bool>::type,
                          B>::value) &&
            (std::is_same<R, T>::value)),
           stencil_fmap>::type,
       const F &f, const G &g, const cxx::tree<U, C, P> &xs, const B &bm,
       const B &bp)
      : node(typename node_t::gmap(),
             [f, g](const leaf<U, C, P> &l, const B &bm, const B &bp) {
               return leaf<T, C, P>(typename leaf<T, C, P>::stencil_fmap(), f,
                                    g, l, bm, bp);
             },
             [f, g](const branch<U, C, P> &b, const B &bm, const B &bp) {
               return branch<T, C, P>(typename branch<T, C, P>::stencil_fmap(),
                                      f, g, b, bm, bp);
             },
             xs.node, bm, bp) {}

  template <typename G, typename B = typename cxx::invoke_of<G, T, bool>::type>
  B get_boundary(const G &g, bool face_upper) const {
    return node.gfoldl(&leaf<T, C, P>::template get_boundary<G>,
                       &branch<T, C, P>::template get_boundary<G>, g,
                       face_upper);
  }

  // monad: join
  template <typename U = T, typename R = typename cxx::kinds<U>::element_type>
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

  // iota
  struct iota : std::tuple<> {};
  static constexpr std::ptrdiff_t default_size() { return 10; }
  template <typename... As, typename F, typename U = T>
  tree(typename std::enable_if<
           std::is_same<U, typename cxx::invoke_of<F, std::ptrdiff_t,
                                                   As...>::type>::value,
           iota>::type,
       const F &f, std::ptrdiff_t imin, std::ptrdiff_t imax,
       std::ptrdiff_t istep, const As &... as)
      : node(div_ceil(imax - imin, istep) <= default_size()
                 ? node_t(leaf<T, C, P>(typename leaf<T, C, P>::iota(), f, imin,
                                        imax, istep, as...))
                 : node_t(branch<T, C, P>(typename branch<T, C, P>::iota(), f,
                                          imin, imax, istep, as...))) {}

  // size
  bool empty_slow() const {
    return foldl([](bool x, const T &v) { return false; }, true);
  }
  std::size_t size_slow() const {
    return foldl([](std::size_t s, const T &v) { return s + 1; },
                 std::size_t(0));
  }
};

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename T, template <typename> class C, template <typename> class P>
struct kinds<tree<T, C, P> > {
  typedef T element_type;
  template <typename U> using constructor = tree<U, C, P>;
};
template <typename T> struct is_tree : std::false_type {};
template <typename T, template <typename> class C, template <typename> class P>
struct is_tree<tree<T, C, P> > : std::true_type {};

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
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_tree<As>::type...>::type>
C<R> fmap(const F &f, const cxx::tree<T, C1, P> &xs, const As &... as) {
  return C<R>(typename C<R>::fmap(), f, xs, as...);
}

template <typename T, template <typename> class C1, template <typename> class P,
          typename B, typename F, typename G, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, B, B>::type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value,
    C<R> >::type
stencil_fmap(const F &f, const G &g, const cxx::tree<T, C1, P> &xs, const B &bm,
             const B &bp) {
  return C<R>(typename C<R>::stencil_fmap(), f, g, xs, bm, bp);
}

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
          typename CT = typename cxx::kinds<CCT>::element_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const cxx::tree<cxx::tree<T, C1, P>, C1, P> &xss) {
  return xss.join();
}

// TODO: Implement this directly
template <typename T, template <typename> class C1, template <typename> class P,
          typename F, typename CT = cxx::tree<T, C1, P>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
C<R> bind(const cxx::tree<T, C1, P> &xs, const F &f) {
  return join(fmap(f, xs));
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

// iota

template <template <typename> class C, typename... As, typename F,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_tree<C<T> >::value, C<T> >::type
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  return C<T>(typename C<T>::iota(), f, imin, imax, istep, as...);
}
}

#endif // #ifndef CXX_TREE_HH
