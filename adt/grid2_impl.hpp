#ifndef ADT_GRID2_IMPL_HPP
#define ADT_GRID2_IMPL_HPP

#include "grid2_decl.hpp"

#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <cxx/cassert.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <fun/fun_impl.hpp>

#include <cereal/access.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace adt {

template <typename C, typename T, std::size_t D> class grid2 {
public:
  static_assert(
      std::is_same<typename fun::fun_traits<C>::value_type, adt::dummy>::value,
      "");

  typedef C container_dummy;
  template <typename U>
  using container_constructor =
      typename fun::fun_traits<C>::template constructor<U>;
  typedef T value_type;
  typedef index_t<D> index_type;
  typedef range_t<D> range_type;
  typedef space_t<D> space_type;
  typedef container_constructor<T> container_type;

  static constexpr std::ptrdiff_t rank = D;

  template <typename C1, typename T1, std::size_t D1> friend class grid2;

private:
  space_type space;
  container_type data;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    // TODO: Serialize only the accessible part of data
    ar(space, data);
  }

public:
  bool empty() const { return space.empty(); }
  std::size_t size() const { return space.size(); }
  range_type active() const { return space.active(); }

  bool invariant() const {
    return fun::msize(data) == space.allocated().size();
  }

  grid2() : space(), data(fun::accumulator<container_type>(size()).finalize()) {
    cxx_assert(invariant());
  }

  grid2(const index_type &shape, const container_type &data)
      : space(shape), data(data) {
    cxx_assert(invariant());
  }
  grid2(const index_type &shape, container_type &&data)
      : space(shape), data(std::move(data)) {
    cxx_assert(invariant());
  }

  grid2(const space_type &active, const container_type &data)
      : space(active), data(data) {
    cxx_assert(invariant());
  }
  grid2(const space_type &active, container_type &&data)
      : space(active), data(std::move(data)) {
    cxx_assert(invariant());
  }

  grid2(const grid2 &) = default;
  grid2(grid2 &&) = default;
  grid2 &operator=(const grid2 &) = default;
  grid2 &operator=(grid2 &&) = default;
  void swap(grid2 &other) {
    using std::swap;
    swap(space, other.space);
    swap(data, other.data);
  }

  // TODO: avoid these

  decltype(auto) head() const {
    return fun::getIndex(data, space.linear(active().imin()));
  }
  decltype(auto) last() const {
    return fun::getIndex(
        data, space.linear(active().imax() - adt::set<index_type>(1)));
  }

  // iotaMap
  // TODO: avoid this

  struct iotaMap {};

  template <typename F, typename... Args, std::size_t D1 = D,
            std::enable_if_t<D1 == 1> * = nullptr>
  grid2(iotaMap, F &&f, const adt::irange_t &inds, Args &&... args)
      : grid2(iotaMap(),
              [&](const index_type &ipos) {
                return cxx::invoke(std::forward<F>(f), ipos[0],
                                   std::forward<Args>(args)...);
              },
              adt::range_t<D>(inds)) {}

  template <typename F, typename... Args>
  grid2(iotaMap, F &&f, const adt::range_t<D> &inds, Args &&... args)
      : space(inds) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, index_type, Args...>, T>::value, "");
    fun::accumulator<container_constructor<T>> acc(size());
    active().loop([&](const index_type &ipos) {
      acc[space.linear(ipos)] =
          cxx::invoke(std::forward<F>(f), ipos, std::forward<Args>(args)...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // fmap

  struct fmap {};

  template <typename F, typename T1, typename... Args>
  grid2(fmap, F &&f, const grid2<C, T1, D> &xs, Args &&... args)
      : space(xs.active()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
    fun::accumulator<container_type> acc(size());
    active().loop([&](const index_type &ipos) {
      acc[space.linear(ipos)] = cxx::invoke(
          std::forward<F>(f), fun::getIndex(xs.data, xs.space.linear(ipos)),
          std::forward<Args>(args)...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  struct fmap2 {};

  template <typename F, typename T1, typename T2, typename... Args>
  grid2(fmap2, F &&f, const grid2<C, T1, D> &xs, const grid2<C, T2, D> &ys,
        Args &&... args)
      : space(xs.active()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, Args...>, T>::value,
                  "");
    cxx_assert(ys.active() == xs.active());
    fun::accumulator<container_type> acc(size());
    active().loop([&](const index_type &ipos) {
      acc[space.linear(ipos)] = cxx::invoke(
          std::forward<F>(f), fun::getIndex(xs.data, xs.space.linear(ipos)),
          fun::getIndex(ys.data, ys.space.linear(ipos)),
          std::forward<Args>(args)...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  struct fmap3 {};

  template <typename F, typename T1, typename T2, typename T3, typename... Args>
  grid2(fmap3, F &&f, const grid2<C, T1, D> &xs, const grid2<C, T2, D> &ys,
        const grid2<C, T3, D> &zs, Args &&... args)
      : space(xs.active()) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, T2, T3, Args...>, T>::value, "");
    cxx_assert(ys.active() == xs.active());
    cxx_assert(zs.active() == xs.active());
    fun::accumulator<container_type> acc(size());
    active().loop([&](const index_type &ipos) {
      acc[space.linear(ipos)] = cxx::invoke(
          std::forward<F>(f), fun::getIndex(xs.data, xs.space.linear(ipos)),
          fun::getIndex(ys.data, ys.space.linear(ipos)),
          fun::getIndex(zs.data, zs.space.linear(ipos)),
          std::forward<Args>(args)...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // boundary

  struct boundary {};

  grid2(boundary, const grid2 &xs, std::ptrdiff_t f, std::ptrdiff_t d)
      : space(xs.space.boundary(f, d)), data(xs.data) {
    cxx_assert(invariant());
  }

  // fmapStencil

  struct fmapStencil {};

  template <typename F, typename G, typename T1, typename B, typename... Args>
  grid2(fmapStencil, F &&f, G &&g, const grid2<C, T1, D> &xs,
        const std::array<std::array<grid2<C, B, D>, D>, 2> &bss,
        Args &&... args)
      : space(xs.active()) {
    typedef cxx::invoke_of_t<F, T1, std::array<std::array<B, D>, 2>, Args...> R;
    static_assert(std::is_same<R, T>::value, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t, std::ptrdiff_t> BR;
    static_assert(std::is_same<BR, B>::value, "");
    for (int f = 0; f < 2; ++f)
      for (int d = 0; d < int(D); ++d)
        cxx_assert(bss[f][d].active() == active().boundary(f, d, true));
    fun::accumulator<container_type> acc(size());
    active().loop_bnd([&](const index_type &ipos,
                          const std::array<std::array<bool, D>, 2> &isbnd) {
      std::array<std::array<B, D>, 2> bs;
      for (int f = 0; f < 2; ++f)
        for (int d = 0; d < int(D); ++d) {
          index_type jpos(ipos - adt::offset<index_type>(f, d));
          bs[f][d] =
              isbnd[f][d]
                  ? fun::getIndex(bss[f][d].data, bss[f][d].space.linear(jpos))
                  : cxx::invoke(std::forward<G>(g),
                                fun::getIndex(xs.data, xs.space.linear(jpos)),
                                f, d);
        }
      acc[space.linear(ipos)] = cxx::invoke(
          std::forward<F>(f), fun::getIndex(xs.data, xs.space.linear(ipos)), bs,
          std::forward<Args>(args)...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // foldMap

  template <typename F, typename Op, typename Z, typename... Args,
            typename R = cxx::invoke_of_t<F, T, Args...>>
  R foldMap(F &&f, Op &&op, Z &&z, Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    R r(std::forward<Z>(z));
    active().loop([&](const index_type &ipos) {
      r = cxx::invoke(std::forward<Op>(op), std::move(r),
                      cxx::invoke(std::forward<F>(f),
                                  fun::getIndex(data, space.linear(ipos)),
                                  std::forward<Args>(args)...));
    });
    return r;
  }

  template <typename F, typename Op, typename Z, typename T2, typename... Args,
            typename R = cxx::invoke_of_t<F, T, T2, Args...>>
  R foldMap2(F &&f, Op &&op, Z &&z, const grid2<C, T2, D> &ys,
             Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    cxx_assert(ys.active() == active());
    R r(std::forward<Z>(z));
    active().loop([&](const index_type &ipos) {
      r = cxx::invoke(std::forward<Op>(op), std::move(r),
                      cxx::invoke(std::forward<F>(f),
                                  fun::getIndex(data, space.linear(ipos)),
                                  fun::getIndex(ys.data, ys.space.linear(ipos)),
                                  std::forward<Args>(args)...));
    });
    return r;
  }

  // dump
  fun::ostreamer dump() const {
    std::ostringstream os;
    os << "grid2{";
    active().loop([&](const index_type &ipos) {
      for (int d = 0; d < int(D); ++d) {
        if (ipos[d] > active().imin()[d])
          break;
        os << "[";
      }
      os << fun::getIndex(data, space.linear(ipos)) << ",";
      for (int d = 0; d < int(D); ++d) {
        if (ipos[d] < active().imax()[d] - 1)
          break;
        os << "],";
      }
    });
    os << "}";
    return fun::ostreamer(os.str());
  }
};
template <typename C, typename T, std::size_t D>
void swap(grid2<C, T, D> &x, grid2<C, T, D> &y) {
  x.swap(y);
}
}

#define ADT_GRID2_IMPL_HPP_DONE
#endif // #ifdef ADT_GRID2_IMPL_HPP
#ifndef ADT_GRID2_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
