#ifndef ADT_TREE_HPP
#define ADT_TREE_HPP

#include <adt/array.hpp>
#include <adt/either.hpp>
#include <adt/dummy.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>

#include <cereal/access.hpp>
#include <cereal/types/tuple.hpp>

#include <algorithm>
#include <cassert>
#include <tuple>
#include <vector>

namespace adt {

template <typename C, typename T> class tree {
  // data Tree a = Leaf (C a) | Node (C (Tree a))

  static_assert(
      std::is_same<typename fun::fun_traits<C>::value_type, adt::dummy>::value,
      "");

  template <typename C1, typename T1> friend class tree;

  static constexpr std::ptrdiff_t max_leaf_size() { return 10; }
  static constexpr std::ptrdiff_t max_branch_size() { return 10; }

public:
  typedef C container_dummy;
  template <typename U>
  using container_constructor =
      typename fun::fun_traits<C>::template constructor<U>;
  typedef T value_type;

private:
  typedef adt::either<container_constructor<T>, container_constructor<tree>>
      either_t;
  either_t subtrees;

  template <typename... Args> static auto make_left(Args &&... args) {
    return adt::make_left<container_constructor<T>,
                          container_constructor<tree>>(
        std::forward<Args>(args)...);
  }
  template <typename... Args> static auto make_right(Args &&... args) {
    return adt::make_right<container_constructor<T>,
                           container_constructor<tree>>(
        std::forward<Args>(args)...);
  }

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(subtrees); }

public:
  tree() : subtrees(make_left(fun::mzero<C, T>())) {}
  tree(const T &x) : subtrees(make_left(fun::munit<C>(x))) {}
  tree(T &&x) : subtrees(make_left(fun::munit<C>(std::move(x)))) {}

  tree(const container_constructor<T> &xs) : subtrees(make_left(xs)) {}
  tree(container_constructor<T> &&xs) : subtrees(make_left(std::move(xs))) {}

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

  tree(const container_constructor<tree> &xss) : subtrees(make_right(xss)) {}
  tree(container_constructor<tree> &&xss)
      : subtrees(make_right(std::move(xss))) {}

  bool invariant() const {
    // TODO: Check that subtrees don't have dummy leaves
    return subtrees.left()
               ? subtrees.get_left().size() <= max_leaf_size()
               : subtrees.get_right().size() > 1 &&
                     subtrees.get_right().size() <= max_branch_size();
  }

  bool empty() const {
    return subtrees.left() && fun::mempty(subtrees.get_left());
  }

  // Note: We cannot return const T& since we don't know the return
  // type of fun::head. We cannot use decltype(auto) since the
  // function is recursive. We cannot use decltype(fun::head(...))
  // since fun::head may not exist for the container C.
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
                   std::plus<std::size_t>(), 0);
  }

  struct iotaMap {};

private:
  static constexpr auto range_count(std::ptrdiff_t imin, std::ptrdiff_t imax,
                                    std::ptrdiff_t istep) {
    assert(istep > 0);
    return std::max(ptrdiff_t(0), cxx::div_floor(imax - imin, istep).quot);
  }

  static constexpr auto range_stride(std::ptrdiff_t icount) {
    assert(icount > 1);
    std::ptrdiff_t istride = 1;
    std::ptrdiff_t nsteps = cxx::div_ceil(icount, istride).quot;
    while (nsteps > max_branch_size()) {
      istride *= max_branch_size();
      nsteps = cxx::div_ceil(icount, istride).quot;
    }
    assert(nsteps > 1);
    assert(istride >= 1);
    assert(istride < icount);
    assert(istride * max_branch_size() >= icount);
    return istride;
  }

  static constexpr auto range_steps(std::ptrdiff_t icount) {
    return cxx::div_ceil(icount, range_stride(icount)).quot;
  }

  struct detail_iotaMap_leaf : std::tuple<> {
    template <typename F, typename... Args>
    auto operator()(std::ptrdiff_t i, std::ptrdiff_t imin, std::ptrdiff_t istep,
                    F &&f, Args &&... args) const {
      return cxx::invoke(std::forward<F>(f), imin + i * istep,
                         std::forward<Args>(args)...);
    }
  };

  struct detail_iotaMap_branch : std::tuple<> {
    template <typename F, typename... Args>
    auto operator()(std::ptrdiff_t i, std::ptrdiff_t parent_imin,
                    std::ptrdiff_t parent_imax, std::ptrdiff_t parent_istride,
                    std::ptrdiff_t istep, F &&f, Args &&... args) const {
      std::ptrdiff_t imin = parent_imin + i * parent_istride;
      std::ptrdiff_t imax = std::min(parent_imax, imin + parent_istride);
      return tree(iotaMap(), std::forward<F>(f), imin, imax, istep,
                  std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename... Args>
  tree(iotaMap, F &&f, std::ptrdiff_t imin, std::ptrdiff_t imax,
       std::ptrdiff_t istep, Args &&... args)
      : subtrees(
            range_count(imin, imax, istep) <= max_leaf_size()
                ? make_left(fun::iotaMap<C>(detail_iotaMap_leaf(),
                                            range_count(imin, imax, istep),
                                            imin, istep, std::forward<F>(f),
                                            std::forward<Args>(args)...))
                : make_right(fun::iotaMap<C>(
                      detail_iotaMap_branch(),
                      range_steps(range_count(imin, imax, istep)), imin, imax,
                      range_stride(range_count(imin, imax, istep)) * istep,
                      istep, std::forward<F>(f),
                      std::forward<Args>(args)...))) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
  }

private:
  template <std::size_t D>
  static constexpr auto range_count(const index_t<D> &imin,
                                    const index_t<D> &imax,
                                    const index_t<D> &istep) {
    assert(adt::all(adt::ge(istep, 0)));
    return adt::max(adt::array_zero<std::ptrdiff_t, D>(),
                    adt::div_quot(adt::div_floor(imax - imin, istep)));
  }

  template <std::size_t D>
  static constexpr auto range_stride(const index_t<D> &icount) {
    assert(adt::all(adt::ge(icount, 1)));
    auto istride = adt::one<index_t<D>>();
    auto nsteps = adt::prod(adt::div_quot(adt::div_ceil(icount, istride)));
    while (nsteps > max_branch_size()) {
      istride *= max_branch_size();
      nsteps = adt::prod(adt::div_quot(adt::div_ceil(icount, istride)));
    }
    assert(nsteps > 1);
    assert(adt::all(adt::ge(istride, 1)));
    assert(adt::all(adt::lt(istride, icount)));
    assert(adt::all(adt::ge(istride * max_branch_size(), icount)));
    return istride;
  }

  template <std::size_t D>
  static constexpr auto range_steps(const index_t<D> &icount) {
    return adt::div_quot(adt::div_ceil(icount, range_stride(icount)));
  }

  struct detail_iotaMap1_leaf : std::tuple<> {
    template <std::size_t D, typename F, typename... Args>
    auto operator()(const index_t<D> &i, const index_t<D> &imin,
                    const index_t<D> &istep, F &&f, Args &&... args) const {
      return cxx::invoke(std::forward<F>(f), imin + i * istep,
                         std::forward<Args>(args)...);
    }
  };

  struct detail_iotaMap1_branch : std::tuple<> {
    template <std::size_t D, typename F, typename... Args>
    auto operator()(const index_t<D> &i, const index_t<D> &parent_imin,
                    const index_t<D> &parent_imax,
                    const index_t<D> &parent_istride, const index_t<D> &istep,
                    F &&f, Args &&... args) const {
      const index_t<D> imin = parent_imin + i * parent_istride;
      const index_t<D> imax = adt::min(parent_imax, imin + parent_istride);
      return tree(iotaMap(), std::forward<F>(f), imin, imax, istep,
                  std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, std::size_t D, typename... Args>
  tree(iotaMap, F &&f, const index_t<D> &imin, const index_t<D> &imax,
       const index_t<D> &istep, Args &&... args)
      : subtrees(
            adt::prod(range_count(imin, imax, istep)) <= max_leaf_size()
                ? make_left(fun::iotaMap<C>(detail_iotaMap1_leaf(),
                                            range_count(imin, imax, istep),
                                            imin, istep, std::forward<F>(f),
                                            std::forward<Args>(args)...))
                : make_right(fun::iotaMap<C>(
                      detail_iotaMap1_branch(),
                      range_steps(range_count(imin, imax, istep)), imin, imax,
                      range_stride(range_count(imin, imax, istep)) * istep,
                      istep, std::forward<F>(f),
                      std::forward<Args>(args)...))) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, index_t<D>, Args...>, T>::value, "");
  }

  struct fmap {};

private:
  struct detail_fmap : std::tuple<> {
    template <typename T1, typename F, typename... Args>
    auto operator()(const tree<C, T1> &xs, F &&f, Args &&... args) const {
      return tree(fmap(), std::forward<F>(f), xs, std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename T1, typename... Args>
  tree(fmap, F &&f, const tree<C, T1> &xs, Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? make_left(fun::fmap(std::forward<F>(f),
                                           xs.subtrees.get_left(),
                                           std::forward<Args>(args)...))
                     : make_right(fun::fmap(
                           detail_fmap(), xs.subtrees.get_right(),
                           std::forward<F>(f), std::forward<Args>(args)...))) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
  }

  struct fmap2 {};

private:
  struct detail_fmap2 : std::tuple<> {
    template <typename T1, typename T2, typename F, typename... Args>
    auto operator()(const tree<C, T1> &xs, const tree<C, T2> &ys, F &&f,
                    Args &&... args) const {
      return tree(fmap2(), std::forward<F>(f), xs, ys,
                  std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename T1, typename T2, typename... Args>
  tree(fmap2, F &&f, const tree<C, T1> &xs, const tree<C, T2> &ys,
       Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? make_left(fun::fmap2(
                           std::forward<F>(f), xs.subtrees.get_left(),
                           ys.subtrees.get_left(), std::forward<Args>(args)...))
                     : make_right(fun::fmap2(
                           detail_fmap2(), xs.subtrees.get_right(),
                           ys.subtrees.get_right(), std::forward<F>(f),
                           std::forward<Args>(args)...))) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, Args...>, T>::value,
                  "");
  }

  struct boundary {};

  template <typename C1>
  tree(boundary, const tree<C1, T> &xs, std::ptrdiff_t i)
      : subtrees(
            xs.subtrees.left()
                ? make_left(fun::boundary(xs.subtrees.get_left(), i))
                : make_right(
                      // TODO: Use boundaryMap instead
                      fun::fmap([](const tree<C1, T> &xs, std::ptrdiff_t i) {
                        return tree(boundary(), xs, i);
                      }, fun::boundary(xs.subtrees.get_right(), i), i))) {
    static_assert(
        std::is_same<typename fun::fun_traits<C1>::boundary_dummy, C>::value,
        "");
  }

  struct boundaryMap {};

private:
  struct boundaryMap_branch {
    template <typename T1, typename F, typename... Args>
    auto operator()(const T1 &xs, F &&f, std::ptrdiff_t i,
                    Args &&... args) const {
      return tree(boundaryMap(), std::forward<F>(f), xs, i,
                  std::forward<Args>(args)...);
    }
  };

public:
  template <typename F, typename C1, typename T1, typename... Args>
  tree(boundaryMap, F &&f, const tree<C1, T1> &xs, std::ptrdiff_t i,
       Args &&... args)
      : subtrees(
            xs.subtrees.left()
                ? fun::boundaryMap(std::forward<F>(f), xs.subtrees.get_left(),
                                   i, std::forward<Args>(args)...)
                : fun::boundaryMap(boundaryMap_branch(), xs.subtrees.right(), i,
                                   std::forward<F>(f), i,
                                   std::forward<Args>(args)...)) {}

  struct fmapStencil {};

private:
  struct detail_fmapStencil_left_f : std::tuple<> {
    template <typename T1, typename BM, typename BP, typename F,
              typename... Args>
    auto operator()(const T1 &x, std::size_t bdirs, BM &&bm, BP &&bp, F &&f,
                    std::size_t bdirmask, Args &&... args) const {
      return cxx::invoke(std::forward<F>(f), x, bdirmask & bdirs,
                         std::forward<BM>(bm), std::forward<BP>(bp),
                         std::forward<Args>(args)...);
    }
  };
  struct detail_fmapStencil_f : std::tuple<> {
    template <typename T1, typename BM, typename BP, typename F, typename G,
              typename... Args>
    auto operator()(const tree<C, T1> &xs, std::size_t bdirs, BM &&bm, BP &&bp,
                    F &&f, G &&g, std::size_t bdirmask, Args &&... args) const {
      return tree(fmapStencil(), std::forward<F>(f), std::forward<G>(g), xs,
                  bdirmask & bdirs, std::forward<BM>(bm), std::forward<BP>(bp),
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
  tree(fmapStencil, F &&f, G &&g, const tree<C, T1> &xs, std::size_t bdirmask,
       BM &&bm, BP &&bp, Args &&... args)
      : subtrees(xs.subtrees.left()
                     ? make_left(fun::fmapStencil(
                           detail_fmapStencil_left_f(), std::forward<G>(g),
                           xs.subtrees.get_left(), std::forward<BM>(bm),
                           std::forward<BP>(bp), std::forward<F>(f), bdirmask,
                           std::forward<Args>(args)...))
                     : make_right(fun::fmapStencil(
                           detail_fmapStencil_f(),
                           detail_fmapStencil_g<std::decay_t<G>>{g},
                           xs.subtrees.get_right(), std::forward<BM>(bm),
                           std::forward<BP>(bp), std::forward<F>(f), g,
                           bdirmask, std::forward<Args>(args)...))) {
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, std::size_t, B, B, Args...>,
                     T>::value,
        "");
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
                     ? make_right(xss.subtrees.get_left())
                     : make_right(fun::fmap([](const tree<C, tree> &xss) {
                       return tree(join(), xss);
                     }, xss.subtrees.get_right()))) {}
};
template <typename C, typename T> void swap(tree<C, T> &x, tree<C, T> &y) {
  x.swap(y);
}
}

#define ADT_TREE_HPP_DONE
#endif // #ifdef ADT_TREE_HPP
#ifndef ADT_TREE_HPP_DONE
#error "Cyclic include dependency"
#endif
