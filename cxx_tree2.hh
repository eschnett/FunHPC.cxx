#ifndef CXX_TREE2_HH
#define CXX_TREE2_HH

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_monad_vector.hh"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cxx {

// Tree

template <typename T, template <typename> class arr> struct tree;

// Kinds

template <typename T, template <typename> class arr>
struct kinds<tree<T, arr> > {
  typedef T value_type;
  template <typename U> using constructor = tree<U, arr>;
};
template <typename T> struct is_tree : std::false_type {};
template <typename T, template <typename> class arr>
struct is_tree<tree<T, arr> > : std::true_type {};

// Tree

template <typename T, template <typename> class arr> struct tree {

  static constexpr std::ptrdiff_t max_arr_size = 10;

  struct leaf;
  struct branch;

  struct leaf {
    arr<T> values;

    bool invariant() const { return true; }
    bool empty() const { return values.empty(); }
    std::size_t size() const { return values.size(); }

    leaf() : values(mzero<arr, T>()) {}
    leaf(const T &v) : values(munit<arr>(v)) {}
    leaf(arr<T> &&vs) : values(std::move(vs)) {}

    void output_tree(std::ostream &os) const {
      os << "[";
      cxx::fmap([&](const T &x) { return os << x << ", ", std::tuple<>(); },
                values);
      os << "]";
    }

    template <typename F, typename... As,
              typename R = cxx::invoke_of_t<F, T, As...> >
    typename tree<R, arr>::leaf fmap(const F &f, const As &... as) const {
      return { cxx::fmap(f, values, as...) };
    }

    template <typename F, typename G, typename B, typename... As,
              typename R = cxx::invoke_of_t<F, T, B, B, As...> >
    typename tree<R, arr>::leaf stencil_fmap(const F &f, const G &g,
                                             const B &bm, const B &bp,
                                             const As &... as) const {
      return { cxx::stencil_fmap(f, g, values, bm, bp, as...) };
    }

    template <typename F, typename Op, typename R>
    R foldMap(const F &f, const Op &op, const R &z) const {
      return cxx::foldMap(f, op, z, values);
    }

    const T &head() const { return cxx::head(values); }
    const T &last() const { return cxx::last(values); }

    template <typename F, typename CR = cxx::invoke_of_t<F, T> >
    typename CR::branch mbind(const F &f) const {
      return { cxx::fmap([&f](const T &x) { return cxx::invoke(f, x); },
                         values) };
    }
  };

  struct branch {
    arr<tree> trees;

    bool invariant() const { return !trees.empty(); }
    bool empty() const { return trees.empty(); }
    std::size_t size() const {
      return cxx::foldMap([](const tree &t) { return t.size(); },
                          std::plus<std::size_t>(), std::size_t(0), trees);
    }

    branch(arr<tree> &&ts) : trees(std::move(ts)) {}

    void output_tree(std::ostream &os) const {
      os << "[";
      cxx::fmap([&](const tree &t) {
                  return t.output_tree(os), os << ", ", std::tuple<>();
                },
                trees);
      os << "]";
    }

    template <typename F, typename... As,
              typename R = cxx::invoke_of_t<F, T, As...> >
    typename tree<R, arr>::branch fmap(const F &f, const As &... as) const {
      return { cxx::fmap(
          [&f](const tree &t, const As &... as) { return t.fmap(f, as...); },
          trees, as...) };
    }

    template <typename F, typename G, typename B, typename... As,
              typename R = cxx::invoke_of_t<F, T, B, B, As...> >
    typename tree<R, arr>::branch stencil_fmap(const F &f, const G &g,
                                               const B &bm, const B &bp,
                                               const As &... as) const {
      return { cxx::stencil_fmap(
          [&f, &g](const tree &t, const B &bm, const B &bp, const As &... as) {
            return t.stencil_fmap(f, g, bm, bp, as...);
          },
          [&g](const tree &t, bool face) {
            return cxx::invoke(g, !face ? t.head() : t.last(), face);
          },
          trees, bm, bp, as...) };
    }

    template <typename F, typename Op, typename R>
    R foldMap(const F &f, const Op &op, const R &z) const {
      return cxx::foldMap([&](const tree &t) { return t.foldMap(f, op, z); },
                          op, z, trees);
    }

    const T &head() const { return cxx::head(trees).head(); }
    const T &last() const { return cxx::last(trees).last(); }

    template <typename F, typename CR = cxx::invoke_of_t<F, T> >
    typename CR::branch mbind(const F &f) const {
      return { cxx::fmap([&f](const tree &t) { return t.mbind(f); }, trees) };
    }
  };

  either<leaf, branch> node;

  bool invariant() const {
    return node.gfoldl(&leaf::invariant, &branch::invariant);
  }
  bool empty() const { return node.gfoldl(&leaf::empty, &branch::empty); }
  std::size_t size() const { return node.gfoldl(&leaf::size, &branch::size); }

  tree() : node(leaf()) {}
  tree(const T &x) : node(leaf(x)) {}
  tree(arr<T> &&xs) : node(leaf(std::move(xs))) {}
  tree(leaf &&l) : node(std::move(l)) {}
  tree(branch &&b) : node(std::move(b)) {}
  tree(either<leaf, branch> &&lb) : node(std::move(lb)) {}

  void output_tree(std::ostream &os) const {
    os << "tree";
    node.gfoldl(&leaf::output_tree, &branch::output_tree, os);
  }

  template <typename F, typename... As>
  static auto iota(const F &f, const iota_range_t &range, const As &... as) {
    if (range.local.size() <= max_arr_size)
      return tree(leaf(cxx::iota<arr>(f, range, as...)));
    iota_range_t coarse_range(range.global, range.local.imin, range.local.imax,
                              range.local.istep * max_arr_size);
    return tree(branch(cxx::iota<arr>(
        [&f, &range](std::ptrdiff_t i, const As &... as) {
          iota_range_t fine_range(
              range.global, i,
              std::min(range.local.imax, i + range.local.istep * max_arr_size),
              range.local.istep);
          return iota(f, fine_range, as...);
        },
        coarse_range, as...)));
  }

  template <typename F, typename... As,
            typename R = cxx::invoke_of_t<F, T, As...> >
  tree<R, arr> fmap(const F &f, const As &... as) const {
    return { cxx::gmap([&](const leaf &l) { return l.fmap(f, as...); },
                       [&](const branch &b) { return b.fmap(f, as...); },
                       node) };
  }

  template <typename F, typename G, typename B, typename... As,
            typename R = cxx::invoke_of_t<F, T, B, B, As...> >
  tree<R, arr> stencil_fmap(const F &f, const G &g, const B &bm, const B &bp,
                            const As &... as) const {
    return { cxx::gmap(
        [&](const leaf &l) { return l.stencil_fmap(f, g, bm, bp, as...); },
        [&](const branch &b) { return b.stencil_fmap(f, g, bm, bp, as...); },
        node) };
  }

  template <typename F, typename Op, typename R>
  R foldMap(const F &f, const Op &op, const R &z) const {
    return node.gfoldl([&](const leaf &l) { return l.foldMap(f, op, z); },
                       [&](const branch &b) { return b.foldMap(f, op, z); });
  }

  const T &head() const { return node.gfoldl(&leaf::head, &branch::head); }
  const T &last() const { return node.gfoldl(&leaf::last, &branch::last); }

  template <typename F, typename CR = cxx::invoke_of_t<F, T> >
  CR mbind(const F &f) const {
    return { node.gfoldl([&](const leaf &l) { return l.mbind(f); },
                         [&](const branch &b) { return b.mbind(f); }) };
  }

  template <typename... Ts> static auto msome(Ts &&... xs) {
    return tree(cxx::msome<arr>(std::forward<Ts>(xs)...));
  }

  template <typename... Ts> auto mplus(const tree<Ts, arr> &... xss) const {
    return tree(branch(cxx::msome<arr>(*this, xss...)));
  }
};

template <typename T, template <typename> class arr>
void output_tree(std::ostream &os, const tree<T, arr> &t) {
  t.output_tree(os);
}

template <typename T, template <typename> class arr>
std::ostream &operator<<(std::ostream &os, const tree<T, arr> &t) {
  return os << "tree[" << foldMap([](const T &x) {
                                    std::ostringstream os;
                                    os << x << ", ";
                                    return os.str();
                                  },
                                  [](const std::string &x,
                                     const std::string &y) { return x + y; },
                                  std::string(), t) << "]";
}

// Iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...>,
          std::enable_if_t<cxx::is_tree<C<T> >::value> * = nullptr>
auto iota(const F &f, const iota_range_t &range, const As &... as) {
  return C<T>::iota(f, range, as...);
}

// Functor

template <typename F, typename T, template <typename> class arr, typename... As>
auto fmap(const F &f, const tree<T, arr> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  return xs.fmap(f, as...);
}

template <typename F, typename G, typename T, template <typename> class arr,
          typename B, typename... As>
auto stencil_fmap(const F &f, const G &g, const tree<T, arr> &xs, const B &bm,
                  const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(std::is_same<cxx::invoke_of_t<G, T, bool>, B>::value, "");
  return xs.stencil_fmap(f, g, bm, bp, as...);
}

// Foldable

template <typename F, typename Op, typename R, typename T,
          template <typename> class arr>
auto foldMap(const F &f, const Op &op, const R &z, const tree<T, arr> &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return xs.foldMap(f, op, z);
}

template <typename Op, typename T, template <typename> class arr>
auto fold(const Op &op, const T &z, const tree<T, arr> &xs) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, T, T>, T>::value, "");
  return xs.foldMap([](const T &x) { return x; }, op, z);
}

template <typename T, template <typename> class arr>
const T &head(const tree<T, arr> &xs) {
  return xs.head();
}
template <typename T, template <typename> class arr>
const T &last(const tree<T, arr> &xs) {
  return xs.last();
}

// Monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type,
          std::enable_if_t<is_tree<C<T> >::value> * = nullptr>
auto munit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As,
          std::enable_if_t<is_tree<C<T> >::value> * = nullptr>
auto mmake(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, template <typename> class arr, typename F>
auto mbind(const tree<T, arr> &xs, const F &f) {
  typedef cxx::invoke_of_t<F, T> CR;
  static_assert(is_tree<CR>::value, "");
  // return mjoin(fmap(f, xs));
  return xs.mbind(f);
}

template <typename T, template <typename> class arr>
auto mjoin(const tree<tree<T, arr>, arr> &xss) {
  // return xss.mjoin();
  return mbind(xss, [](const tree<T, arr> &xs) { return xs; });
}

template <template <typename> class C, typename T,
          std::enable_if_t<is_tree<C<T> >::value> * = nullptr>
auto mzero() {
  return C<T>();
}

template <template <typename> class C, typename T, typename... Ts,
          typename T1 = std::decay_t<T>,
          std::enable_if_t<is_tree<C<T1> >::value> * = nullptr>
auto msome(T &&x, Ts &&... xs) {
  static_assert(cxx::all<std::is_same<std::decay_t<Ts>, T1>::value...>::value,
                "");
  return C<T1>::msome(std::forward<T>(x), std::forward<Ts>(xs)...);
}

template <typename T, typename... Ts, template <typename> class arr>
auto mplus(const tree<T, arr> &xs, const tree<Ts, arr> &... xss) {
  static_assert(cxx::all<std::is_same<T, Ts>::value...>::value, "");
  return xs.mplus(xss...);
}
}

#endif // CXX_TREE2_HH
