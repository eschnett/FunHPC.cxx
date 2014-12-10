#ifndef CXX_TREE2_HH
#define CXX_TREE2_HH

#include "rpc_action.hh"
#include "rpc_call.hh"

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_monad_vector.hh"
#include "cxx_shape.hh"

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

  template <typename U> using leaf1 = typename tree<U, arr>::leaf;
  template <typename U> using branch1 = typename tree<U, arr>::branch;

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
    leaf1<R> fmap(const F &f, const As &... as) const {
      return { cxx::fmap(f, values, as...) };
    }

    template <typename F, typename G, typename B, typename... As,
              typename R = cxx::invoke_of_t<F, T, B, B, As...> >
    leaf1<R> stencil_fmap(const F &f, const G &g, const B &bm, const B &bp,
                          const As &... as) const {
      return { cxx::stencil_fmap(f, g, values, bm, bp, as...) };
    }

    template <typename F, typename... As,
              typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> >
    leaf1<R> boundary(const F &f, std::ptrdiff_t dir, bool face,
                      const As &... as) const {
      return { cxx::boundary(f, values, dir, face, as...) };
    }

    template <typename F, typename G, std::ptrdiff_t D, typename... As,
              typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>,
              typename R = cxx::invoke_of_t<F, T, boundaries<B, D>, As...> >
    leaf1<R> stencil_fmap(const F &f, const G &g,
                          const boundaries<leaf1<B>, D> &bs,
                          const As &... as) const {
      return { cxx::stencil_fmap(
          f, g, values, cxx::fmap([](const auto &b) { return b.values; }, bs),
          as...) };
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
    branch1<R> fmap(const F &f, const As &... as) const {
      return { cxx::fmap(
          [&f](const tree &t, const As &... as) { return t.fmap(f, as...); },
          trees, as...) };
    }

    template <typename F, typename G, typename B, typename... As,
              typename R = cxx::invoke_of_t<F, T, B, B, As...> >
    branch1<R> stencil_fmap(const F &f, const G &g, const B &bm, const B &bp,
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

    template <typename F, typename... As,
              typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> >
    branch1<R> boundary(const F &f, std::ptrdiff_t dir, bool face,
                        const As &... as) const {
      return { cxx::boundary(
          [](const tree &t, std::ptrdiff_t dir, bool face, const F &f,
             const As &... as) { return t.boundary(f, dir, face, as...); },
          trees, dir, face, f, as...) };
    }

    template <typename F, typename G, std::ptrdiff_t D, typename... As,
              typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>,
              typename R = cxx::invoke_of_t<F, T, boundaries<B, D>, As...> >
    branch1<R> stencil_fmap(const F &f, const G &g,
                            const boundaries<branch1<B>, D> &bs,
                            const As &... as) const {
      return { cxx::stencil_fmap(
          [&f, &g](const auto &t, const auto &bs, const As &... as) {
            return t.stencil_fmap(f, g, bs, as...);
          },
          [&g](const auto &t, std::ptrdiff_t dir,
               bool face) { return t.boundary(g, dir, face); },
          trees, cxx::fmap([](const auto &b) { return b.trees; }, bs), as...) };
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

  template <typename F, typename... As,
            std::enable_if_t<!rpc::is_action<F>::value> * = nullptr>
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
  template <typename F, typename... As> struct iota_coarse {
    static auto call(std::ptrdiff_t i, const F &f, const iota_range_t &range,
                     const As &... as) {
      iota_range_t fine_range(
          range.global, i,
          std::min(range.local.imax, i + range.local.istep * max_arr_size),
          range.local.istep);
      return iota(f, fine_range, as...);
    }
    RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(call);
  };
  template <typename F, typename... As,
            std::enable_if_t<rpc::is_action<F>::value> * = nullptr>
  static auto iota(const F &f, const iota_range_t &range, const As &... as) {
    if (range.local.size() <= max_arr_size)
      return tree(leaf(cxx::iota<arr>(f, range, as...)));
    iota_range_t coarse_range(range.global, range.local.imin, range.local.imax,
                              range.local.istep * max_arr_size);
    typedef iota_coarse<F, As...> coarse;
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(coarse::call);
    return tree(branch(cxx::iota<arr>(typename coarse::call_action(),
                                      coarse_range, f, range, as...)));
  }

  template <typename F, std::ptrdiff_t D, typename... As
            /*std::enable_if_t<!rpc::is_action<F>::value> * = nullptr*/>
  static auto iota(const F &f, const grid_region<D> &global_range,
                   const grid_region<D> &range, const As &... as) {
    if (range.size() <= max_arr_size)
      return tree(leaf(cxx::iota<arr>(f, global_range, range, as...)));
    std::ptrdiff_t fact = std::llrint(
        std::floor(std::pow(double(max_arr_size), 1.0 / double(D))));
    grid_region<D> coarse_range(
        (range.imax() - range.imin() + index<D>::set1(fact - 1)) / fact);
    assert(!coarse_range.empty());
    return tree(branch(
        cxx::iota<arr>([&f, &range, fact](const grid_region<D> &global_range,
                                          const index<D> &i, const As &... as) {
                         grid_region<D> fine_range(
                             i, min(range.imax(), i + index<D>::set1(fact)));
                         return iota(f, global_range, fine_range, as...);
                       },
                       global_range, coarse_range, as...)));
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

  template <typename F, typename... As,
            typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> >
  tree<R, arr> boundary(const F &f, std::ptrdiff_t dir, bool face,
                        const As &... as) const {
    return { cxx::gmap(&leaf::template boundary<F, As...>,
                       &branch::template boundary<F, As...>, node, f, dir, face,
                       as...) };
  }

  template <typename F, typename G, std::ptrdiff_t D, typename... As,
            typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>,
            typename R = cxx::invoke_of_t<F, T, boundaries<B, D>, As...> >
  tree<R, arr> stencil_fmap(const F &f, const G &g,
                            const boundaries<tree<B, arr>, D> &bs,
                            const As &... as) const {
    return { cxx::gmap(
        [](const leaf &l, const F &f, const G &g, const auto &bs,
           const As &... as) {
          return l.stencil_fmap(
              f, g, cxx::fmap([](const auto &b) { return b.left(); }, bs),
              as...);
        },
        [](const branch &b, const F &f, const G &g, const auto &bs,
           const As &... as) {
          return b.stencil_fmap(
              f, g, cxx::fmap([](const auto &b) { return b.right(); }, bs),
              as...);
        },
        node, f, g, cxx::fmap([](const auto &b) { return b.node; }, bs),
        as...) };
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
template <typename F, typename... As>
typename tree<T, arr>::template iota_coarse<F, As...>::call_evaluate_export_t
    tree<T, arr>::template iota_coarse<F, As...>::call_evaluate_export =
        call_evaluate_export_init();

// Output

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

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As,
          typename T = cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
          std::enable_if_t<cxx::is_tree<C<T> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  return C<T>::iota(f, global_range, range, as...);
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

template <typename F, typename T, template <typename> class arr, typename... As>
auto boundary(const F &f, const tree<T, arr> &xs, std::ptrdiff_t dir, bool face,
              const As &... as) {
  return xs.boundary(f, dir, face, as...);
}

template <typename F, typename G, typename T, template <typename> class arr,
          typename B, std::ptrdiff_t D, typename... As>
auto stencil_fmap(const F &f, const G &g, const tree<T, arr> &xs,
                  const boundaries<tree<B, arr>, D> &bs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
  static_assert(
      std::is_same<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>, B>::value, "");
  return xs.stencil_fmap(f, g, bs, as...);
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

#define CXX_TREE2_HH_DONE
#else
#ifndef CXX_TREE2_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_TREE2_HH
