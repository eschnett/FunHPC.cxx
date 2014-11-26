#ifndef CXX_TREE2_HH
#define CXX_TREE2_HH

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_monad.hh"
#include "cxx_monad_vector.hh"

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>

namespace cxx {

template <typename T, template <typename> class arr,
          template <typename> class ptr>
struct tree {
  struct subtree;

  struct fmap : std::tuple<> {};

  struct leaf {
    typedef T value_type;
    arr<ptr<T> > values;

    bool invariant() const { return true; }
    bool empty() const { return values.empty(); }
    std::size_t size() const { return values.size(); }

    leaf() : values(mzero<arr, ptr<T> >()) {}
    leaf(const ptr<T> &pv) : values(munit<arr>(pv)) {}
    leaf(arr<ptr<T> > &&apv) : values(std::move(apv)) {}

    template <typename F, typename leaf1>
    leaf(fmap, const F &f, const leaf1 &l)
        : values(cxx::fmap([&f](const auto &pv) { return cxx::fmap(f, pv); },
                           l.values)) {}
  };

  struct branch {
    typedef T value_type;
    arr<ptr<subtree> > subtrees;

    bool invariant() const {
      return !subtrees.empty() &&
             !foldable::any_of(
                 [](const ptr<subtree> &pt) { return pt->empty(); }, subtrees);
    }
    bool empty() const { return subtrees.empty(); }
    std::size_t size() const {
      return foldMap([](const ptr<subtree> &pt) { return pt->size(); },
                     std::plus<std::size_t>(), std::size_t(0), subtrees);
    }

    branch(arr<ptr<subtree> > &&aps) : subtrees(std::move(aps)) {}

    template <typename F, typename branch1>
    branch(fmap, const F &f, const branch1 &b)
        : subtrees(cxx::fmap([&f](const auto &ps) {
                               return cxx::fmap([&f](const auto &s) {
                                                  return subtree(fmap(), f, s);
                                                },
                                                ps);
                             },
                             b.subtrees)) {}
  };

  struct subtree {
    typedef T value_type;
    either<leaf, branch> node;

    bool invariant() const {
      return node.gfoldl(&leaf::invariant, &branch::invariant);
    }
    bool empty() const { return node.gfoldl(&leaf::empty, &branch::empty); }
    std::size_t size() const { return node.gfoldl(&leaf::size, &branch::size); }

    subtree() : node(leaf()) {}
    subtree(const ptr<T> &px) : node(leaf(px)) {}
    subtree(either<leaf, branch> &&lb) : node(std::move(lb)) {}

    template <typename F, typename subtree1>
    subtree(fmap, const F &f, const subtree1 &s)
        : node(cxx::gmap([&f](const auto &l) { return leaf(fmap(), f, l); },
                         [&f](const auto &b) { return branch(fmap(), f, b); },
                         s.node)) {}
  };

  typedef T value_type;
  subtree s;

  bool invariant() const { return s.invariant(); }
  bool empty() const { return s.empty(); }
  std::size_t size() const { return s.size(); }

  tree() : s(subtree()) {}
  tree(const ptr<T> &px) : s(subtree(px)) {}
  tree(const T &x) : tree(munit<ptr>(x)) {}
  tree(subtree &&s) : s(std::move(s)) {}

  template <typename F, typename T1>
  tree(fmap, const F &f, const tree<T1, arr, ptr> &t)
      : s(subtree(fmap(), f, t.s)) {}
};

template <typename F, typename T, template <typename> class arr,
          template <typename> class ptr,
          typename R = typename cxx::invoke_of<F, T>::type>
auto fmap(const F &f, const tree<T, arr, ptr> &t) {
  return tree<R, arr, ptr>(typename tree<R, arr, ptr>::fmap(), f, t);
}
}

#endif // CXX_TREE2_HH
