#ifndef ADT_TREE_HPP
#define ADT_TREE_HPP

#include <adt/either.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <algorithm>
#include <cassert>
#include <tuple>
#include <vector>

namespace adt {

template <template <typename> class C, typename T> class tree {
  // data Tree a = Node [(Tree a, a)] | Empty
  // data Tree a = Node (C (Tree a, a)) | Empty

  template <template <typename> class C1, typename T1> friend class tree;

  static constexpr std::ptrdiff_t max_leaf_size = 10;
  static constexpr std::ptrdiff_t max_branch_size = 10;

  adt::either<C<T>, C<tree>> subtrees;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(subtrees); }

public:
  template <typename U> using constructor = C<U>;
  typedef T value_type;

  tree() : subtrees(adt::make_left<C<T>, C<tree>>(fun::mzero<C, T>())) {}
  tree(const T &x)
      : subtrees(adt::make_left<C<T>, C<tree>>(fun::munit<C>(x))) {}
  tree(T &&x)
      : subtrees(adt::make_left<C<T>, C<tree>>(fun::munit<C>(std::move(x)))) {}
  template <typename... Ts>
  tree(const T &x, const Ts &... ys)
      : subtrees(adt::make_left<C<T>, C<tree>>(fun::msome<C>(x, ys...))) {}

  tree(const tree &other) : subtrees(other.subtrees) {}
  tree(tree &&other) : subtrees() { swap(other); }
  tree &operator=(const tree &other) {
    subtrees = other.subtrees;
    return *this;
  }
  tree &operator=(tree &&other) {
    subtrees = {};
    swap(other);
    return *this;
  }
  void swap(tree &other) {
    using std::swap;
    swap(subtrees, other.subtrees);
  }

  template <typename... Ts>
  tree(const tree &xs, const tree &ys, const tree<C, Ts> &... zss)
      : subtrees(
            adt::make_right<C<T>, C<tree>>(fun::msome<C>(xs, ys, zss...))) {}

  bool invariant() const {
    return subtrees.left() ? subtrees.get_left().size() <= max_leaf_size
                           : subtrees.get_right().size() > 1 &&
                                 subtrees.get_right().size() <= max_branch_size;
  }

  bool empty() const {
    return subtrees.left() && fun::mempty(subtrees.get_left());
  }

  T head() const {
    return subtrees.left() ? fun::head(subtrees.get_left())
                           : fun::head(subtrees.get_right()).head();
  }
  T last() const {
    return subtrees.left() ? fun::last(subtrees.get_left())
                           : fun::last(subtrees.get_right()).last();
  }

  std::size_t size() const {
    return foldMap([](auto) { return std::size_t(1); },
                   [](auto x, auto y) { return x + y; }, 0);
  }

  struct iotaMap {};
  template <typename F, typename... Args>
  tree(iotaMap, F &&f, std::ptrdiff_t imin, std::ptrdiff_t imax,
       std::ptrdiff_t istep, Args &&... args) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
    std::ptrdiff_t icount = cxx::div_ceil(imax - imin, istep).quot;
    assert(icount >= 0);
    if (icount <= max_leaf_size) {
      subtrees = adt::make_left<C<T>, C<tree>>(fun::iotaMap<C>(
          [imin, istep](std::ptrdiff_t i, auto &&f, auto &&... args) {
            return cxx::invoke(f, imin + i * istep, args...);
          },
          icount, std::forward<F>(f), std::forward<Args>(args)...));
    } else {
      std::ptrdiff_t stride = max_leaf_size;
      std::ptrdiff_t num_subtrees = cxx::div_ceil(icount, max_leaf_size).quot;
      while (num_subtrees > max_branch_size) {
        stride *= max_branch_size;
        num_subtrees = cxx::div_ceil(num_subtrees, max_branch_size).quot;
      }
      assert(num_subtrees > 1 && num_subtrees <= max_branch_size);
      subtrees = adt::make_right<C<T>, C<tree>>(fun::iotaMap<C>(
          [ imin, imax, istep, istride = stride * istep ](
              std::ptrdiff_t i, auto &&f, auto &&... args) {
            std::ptrdiff_t imin1 = imin + i * istride;
            std::ptrdiff_t imax1 = std::min(imax, imin1 + istride);
            assert(imin1 < imax);
            return tree(iotaMap(), f, imin1, imax1, istep, args...);
          },
          num_subtrees, std::forward<F>(f), std::forward<Args>(args)...));
    }
  }

  struct fmap {};

private:
  template <typename T1> struct detail_fmap : std::tuple<> {
    template <typename F, typename... Args>
    auto operator()(const tree<C, T1> &xs, F &&f, Args &&... args) {
      return tree(fmap(), f, xs, args...);
    }
  };

public:
  template <typename F, typename T1, typename... Args>
  tree(fmap, F &&f, const tree<C, T1> &xs, Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? adt::make_left<C<T>, C<tree>>(
                           fun::fmap(std::forward<F>(f), xs.subtrees.get_left(),
                                     std::forward<Args>(args)...))
                     : adt::make_right<C<T>, C<tree>>(fun::fmap(
                           detail_fmap<T1>(), xs.subtrees.get_right(),
                           std::forward<F>(f), std::forward<Args>(args)...))) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
  }

  struct fmap2 {};

private:
  template <typename T1, typename T2> struct detail_fmap2 : std::tuple<> {
    template <typename F, typename... Args>
    auto operator()(const tree<C, T1> &xs, const tree<C, T2> &ys, F &&f,
                    Args &&... args) const {
      return tree(fmap2(), f, xs, ys, args...);
    }
  };

public:
  template <typename F, typename T1, typename T2, typename... Args>
  tree(fmap2, F &&f, const tree<C, T1> &xs, const tree<C, T2> &ys,
       Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? adt::make_left<C<T>, C<tree>>(fun::fmap2(
                           std::forward<F>(f), xs.subtrees.get_left(),
                           ys.subtrees.get_left(), std::forward<Args>(args)...))
                     : adt::make_right<C<T>, C<tree>>(fun::fmap2(
                           detail_fmap2<T1, T2>(), xs.subtrees.get_right(),
                           ys.subtrees.get_right(), std::forward<F>(f),
                           std::forward<Args>(args)...))) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, Args...>, T>::value,
                  "");
  }

  struct fmapStencil {};

private:
  struct detail_fmapStencil_f : std::tuple<> {
    template <typename T1, typename BM, typename BP, typename F, typename G,
              typename... Args>
    auto operator()(const tree<C, T1> &xs, BM &&bm, BP &&bp, F &&f, G &&g,
                    Args &&... args) const {
      return tree(fmapStencil(), std::forward<F>(f), std::forward<G>(g), xs,
                  std::forward<BM>(bm), std::forward<BP>(bp),
                  std::forward<Args>(args)...);
    }
  };
  template <typename G> struct detail_fmapStencil_g {
    G g;
    template <typename Archive> void serialize(Archive &ar) { ar(g); }
    template <typename T1>
    auto operator()(const tree<C, T1> &xs, std::ptrdiff_t i) const {
      return cxx::invoke(g, i == 0 ? xs.head() : xs.last(), i);
    }
  };

public:
  template <typename F, typename G, typename T1, typename BM, typename BP,
            typename... Args>
  tree(fmapStencil, F &&f, G &&g, const tree<C, T1> &xs, BM &&bm, BP &&bp,
       Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? adt::make_left<C<T>, C<tree>>(fun::fmapStencil(
                           std::forward<F>(f), std::forward<G>(g),
                           xs.subtrees.get_left(), std::forward<BM>(bm),
                           std::forward<BP>(bp), std::forward<Args>(args)...))
                     : adt::make_right<C<T>, C<tree>>(fun::fmapStencil(
                           detail_fmapStencil_f(),
                           detail_fmapStencil_g<std::decay_t<G>>{g},
                           xs.subtrees.get_right(), std::forward<BM>(bm),
                           std::forward<BP>(bp), std::forward<F>(f), g,
                           std::forward<Args>(args)...))) {
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, B, B, Args...>, T>::value, "");
    static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
    static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  }

private:
  struct detail_foldMap : std::tuple<> {
    template <typename F, typename Op, typename Z, typename... Args>
    auto operator()(const tree &t, F &&f, Op &&op, const Z &z,
                    Args &&... args) const {
      return t.foldMap(std::forward<F>(f), std::forward<Op>(op), z,
                       std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename Op, typename Z, typename... Args,
            typename R = cxx::invoke_of_t<F, T, Args...>>
  R foldMap(F &&f, Op &&op, const Z &z, Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    return subtrees.left()
               ? fun::foldMap(std::forward<F>(f), std::forward<Op>(op), z,
                              subtrees.get_left(), std::forward<Args>(args)...)
               : fun::foldMap(detail_foldMap(), std::forward<Op>(op), z,
                              subtrees.get_right(), std::forward<F>(f),
                              std::forward<Op>(op), z,
                              std::forward<Args>(args)...);
  }

private:
  template <typename T2> struct detail_foldMap2 : std::tuple<> {
    template <typename F, typename Op, typename Z, typename... Args>
    auto operator()(const tree &t, const tree<C, T2> &t2, F &&f, Op &&op,
                    const Z &z, Args &&... args) const {
      return t.foldMap2(std::forward<F>(f), std::forward<Op>(op), z, t2,
                        std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename Op, typename Z, typename T2, typename... Args,
            typename R = cxx::invoke_of_t<F, T, T2, Args...>>
  R foldMap2(F &&f, Op &&op, const Z &z, const tree<C, T2> &ys,
             Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    bool s = subtrees.left();
    assert(ys.subtrees.left() == s);
    return s ? fun::foldMap2(std::forward<F>(f), std::forward<Op>(op), z,
                             subtrees.get_left(), ys.subtrees.get_left(),
                             std::forward<Args>(args)...)
             : fun::foldMap2(detail_foldMap2<T2>(), std::forward<Op>(op), z,
                             subtrees.get_right(), ys.subtrees.get_right(),
                             std::forward<F>(f), std::forward<Op>(op), z,
                             std::forward<Args>(args)...);
  }

  struct join {};
  tree(join, const tree<C, tree> &xss)
      : subtrees(xss.subtrees.left()
                     ? adt::make_right<C<T>, C<tree>>(xss.subtrees.get_left())
                     : adt::make_right<C<T>, C<tree>>(
                           fun::fmap([](const tree<C, tree> &xss) {
                             return tree(join(), xss);
                           }, xss.subtrees.get_right()))) {}
};
template <template <typename> class C, typename T>
void swap(tree<C, T> &x, tree<C, T> &y) {
  x.swap(y);
}
}

#define ADT_TREE_HPP_DONE
#endif // #ifdef ADT_TREE_HPP
#ifndef ADT_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
