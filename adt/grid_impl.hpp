#ifndef ADT_GRID_IMPL_HPP
#define ADT_GRID_IMPL_HPP

#include "grid_decl.hpp"

#include <adt/index.hpp>
#include <cxx/cassert.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <cereal/access.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace adt {

namespace detail {

template <std::size_t D> class index_space {
public:
  typedef adt::index_t<D> index_type;

private:
  index_type m_shape;
  std::ptrdiff_t m_offset;
  // TODO: Make boundaries be the same type as the domain. This
  // ensures that the stride of dimension 0 is always 1, which is much
  // more efficient.
  // TODO: Do this only for grid?
  std::ptrdiff_t m_stride0;
  index_type m_stride;
  std::ptrdiff_t m_allocated;

  template <std::size_t D1> friend class index_space;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(m_shape, m_offset, m_stride0, m_stride, m_allocated);
  }

  static constexpr index_type origin() noexcept {
    return adt::array_zero<std::ptrdiff_t, D>();
  }

  static std::ptrdiff_t make_offset(const index_type &stride,
                                    const index_type &offset) {
    std::ptrdiff_t off = 0;
    std::ptrdiff_t str = 1;
    for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
      cxx_assert(offset[d] >= 0 &&
                 (stride[d] == 0 ? offset[d] == 0 : offset[d] < stride[d]));
      off += offset[d] * str;
      str = stride[d];
    }
    return off;
  }

  // Note: The stride for dimension 0 is always 1, and is not stored.
  // (NOTE: Not any more; see above!) Instead, we store the total
  // number of points.
  static index_type make_stride(const index_type &shape) {
    index_type stride;
    std::ptrdiff_t i = 1;
    for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
      i *= shape[d];
      stride[d] = i;
    }
    cxx_assert(D == 0 || stride[D - 1] == prod(shape));
    return stride;
  }

  static std::ptrdiff_t make_allocated(const index_type &shape) {
    return adt::prod(shape);
  }

public:
  bool invariant() const noexcept {
    if (adt::any(adt::lt(m_shape, 0)))
      return false;
    if (m_offset < 0)
      return false;
    auto off = offset();
    if (adt::any(adt::lt(off, 0)))
      return false;
    if (m_stride0 < 0)
      return false;
    if (adt::any(adt::lt(m_stride, 0)))
      return false;
    for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
      if (stride(d) < 0)
        return false;
      if (stride(d) > 0 && stride(d + 1) % stride(d) != 0)
        return false;
    }
    if (m_allocated < 0)
      return false;
    if (std::ptrdiff_t(m_offset + size()) > m_allocated)
      return false;
    auto alloc = allocated();
    if (adt::any(adt::lt(alloc, 0)))
      return false;
    if (adt::any(adt::gt(off + m_shape, alloc)))
      return false;
    return true;
  }

  index_space() : index_space(origin()) {}
  // TODO: Add padding if desired
  index_space(const index_type &shape) : index_space(shape, origin(), shape) {}
  index_space(const index_type &shape, const index_type &offset,
              const index_type &allocated)
      : m_shape(shape), m_offset(make_offset(make_stride(allocated), offset)),
        m_stride0(1), m_stride(make_stride(allocated)),
        m_allocated(make_allocated(allocated)) {
    cxx_assert(invariant());
  }

  void swap(index_space &other) {
    using std::swap;
    swap(m_shape, other.m_shape);
    swap(m_offset, other.m_offset);
    swap(m_stride0, other.m_stride0);
    swap(m_stride, other.m_stride);
    swap(m_allocated, other.m_allocated);
  }

  const index_type &shape() const { return m_shape; }
  std::size_t size() const { return adt::prod(shape()); }
  bool empty() const { return size() == 0; }

  std::ptrdiff_t stride(std::ptrdiff_t d) const {
    cxx_assert(d >= 0 && d <= std::ptrdiff_t(D));
    if (d == 0)
      return D == 0 ? /*1*/ m_stride0
                    : std::min(/*std::ptrdiff_t(1)*/ m_stride0, m_stride[0]);
    return m_stride[d - 1];
  }
  index_type offset() const {
    index_type r;
    for (std::size_t d = 0; d < D; ++d)
      r[d] = stride(d) == 0 || stride(d + 1) == 0
                 ? 0
                 : (m_offset / stride(d)) % stride(d + 1);
    return r;
  }
  index_type allocated() const {
    index_type r;
    for (std::size_t d = 0; d < D; ++d)
      r[d] = stride(d) == 0 ? 0 : stride(d + 1) / stride(d);
    return r;
  }
  std::ptrdiff_t allocated_size() const { return m_allocated; }

  // Convert a position to a linear index
  std::ptrdiff_t linear(const index_type &i) const {
    std::ptrdiff_t lin = m_offset;
    for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
      cxx_assert(i[d] >= 0 && i[d] < m_shape[d]);
      lin += i[d] * stride(d);
    }
    cxx_assert(lin < m_allocated);
    return lin;
  }

  struct boundary {};
  index_space(boundary, const index_space<D + 1> &old, std::ptrdiff_t i) {
    cxx_assert(i >= 0 && i < 2 * (std::ptrdiff_t(D) + 1));
    auto f = i % 2, d = i / 2;
    m_shape = adt::rmdir(old.m_shape, d);
    auto off = old.shape() * 0;
    off[d] = !f ? 0 : old.shape()[d] - 1;
    m_offset = old.linear(off);
    m_stride0 = d == 0 ? old.m_stride[0] : old.m_stride0;
    m_stride = adt::rmdir(old.m_stride, d == 0 ? 0 : d - 1);
    m_allocated = old.m_allocated;
    cxx_assert(invariant());
  }

  // Multi-dimensional loop
private:
  // terminating case
  template <std::ptrdiff_t d, typename F, typename... Args,
            std::enable_if_t<(d == 0)> * = nullptr>
  void loop_impl(F &&f, const index_type &pos, Args &&... args) const {
    cxx::invoke(std::forward<F>(f), pos, std::forward<Args>(args)...);
  }
  // recursive implementation
  template <std::ptrdiff_t d, typename F, typename... Args,
            std::enable_if_t<(d > 0)> * = nullptr>
  void loop_impl(F &&f, const index_type &pos, Args &&... args) const {
#pragma omp simd
    for (std::ptrdiff_t i = 0; i < std::get<d - 1>(m_shape); ++i)
      loop_impl<d - 1>(f, update<d - 1>(pos, i), args...);
  }

public:
  // starting point
  template <typename F, typename... Args>
  void loop(F &&f, Args &&... args) const {
    static_assert(std::is_void<cxx::invoke_of_t<F, index_type, Args...>>::value,
                  "");
    loop_impl<D>(std::forward<F>(f), origin(), std::forward<Args>(args)...);
  }

  friend std::ostream &operator<<(std::ostream &os, const index_space &is) {
    adt::index_t<D + 1> strides;
    for (std::ptrdiff_t d = 0; d <= std::ptrdiff_t(D); ++d)
      strides[d] = is.stride(d);
    return os << "index_space{m_shape=" << is.m_shape
              << ",m_offset=" << is.m_offset << ",m_stride0=" << is.m_stride0
              << ",m_stride=" << is.m_stride
              << ",m_allocated=" << is.m_allocated << ";stride()=" << strides
              << ",offset()=" << is.offset() << ",allocated=" << is.allocated()
              << "}";
  }
};
template <std::size_t D> void swap(index_space<D> &x, index_space<D> &y) {
  x.swap(y);
}
} // namespace detail

template <typename C, typename T, std::size_t D> class grid {
public:
  static_assert(
      std::is_same<typename fun::fun_traits<C>::value_type, adt::dummy>::value,
      "");

  typedef C container_dummy;
  template <typename U>
  using container_constructor =
      typename fun::fun_traits<C>::template constructor<U>;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = D;
  typedef detail::index_space<D> index_space;
  typedef typename index_space::index_type index_type;

  template <typename C1, typename T1, std::size_t D1> friend class grid;

private:
  index_space indexing;
  container_constructor<T> data;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    // TODO: Serialize only the accessible part of data
    ar(indexing, data);
  }

public:
  bool empty() const { return indexing.empty(); }
  std::size_t size() const { return indexing.size(); }
  index_type shape() const { return indexing.shape(); }

private:
  bool invariant0() const {
    return indexing.allocated_size() == std::ptrdiff_t(fun::msize(data));
  }

public:
  bool invariant() const {
    auto inv = invariant0();
    if (!inv)
      std::cout << "grid::invariant: indexing=" << indexing
                << ", data.size()=" << fun::msize(data) << "\n";
    return inv;
  }

  grid()
      : indexing(),
        data(fun::accumulator<container_constructor<T>>(indexing.size())
                 .finalize()) {
    cxx_assert(invariant());
  }

  grid(const index_type &shape, const container_constructor<T> &data)
      : indexing(shape), data(data) {
    cxx_assert(invariant());
  }
  grid(const index_type &shape, container_constructor<T> &&data)
      : indexing(shape), data(std::move(data)) {
    cxx_assert(invariant());
  }

  grid(const grid &) = default;
  grid(grid &&) = default;
  grid &operator=(const grid &) = default;
  grid &operator=(grid &&) = default;
  void swap(grid &other) {
    using std::swap;
    swap(indexing, other.indexing);
    swap(data, other.data);
  }

  const T &head() const {
    return fun::getIndex(data, indexing.linear(adt::set<index_type>(0)));
  }
  const T &last() const {
    return fun::getIndex(
        data, indexing.linear(indexing.shape() - adt::set<index_type>(1)));
  }

  // iotaMap

  struct iotaMap {};

  template <typename F, typename... Args, std::size_t D2 = D,
            std::enable_if_t<D2 == 0> * = nullptr>
  grid(iotaMap, F &&f, const adt::irange_t &inds, Args &&... args)
      : indexing(index_type{{}}),
        data(fun::iotaMap<C>(std::forward<F>(f), inds,
                             std::forward<Args>(args)...)) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
    cxx_assert(invariant());
  }

  template <typename F, typename... Args, std::size_t D2 = D,
            std::enable_if_t<D2 == 1> * = nullptr>
  grid(iotaMap, F &&f, const adt::irange_t &inds, Args &&... args)
      : indexing(index_type{{inds.shape()}}),
        data(fun::iotaMap<C>(std::forward<F>(f), inds,
                             std::forward<Args>(args)...)) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
    cxx_assert(invariant());
  }

  struct iotaMapMulti {};

  template <typename F, typename... Args>
  grid(iotaMapMulti, F &&f, const adt::steprange_t<D> &inds, Args &&... args)
      : indexing(inds.shape()) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, index_type, Args...>, T>::value, "");
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    indexing.loop([&](const index_type &i) {
      acc[indexing.linear(i)] = cxx::invoke(f, i, args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // fmap

  struct fmap {};

  template <typename F, typename T1, typename... Args>
  grid(fmap, F &&f, const grid<C, T1, D> &xs, Args &&... args)
      : indexing(xs.shape()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    indexing.loop([&](const index_type &i) {
      acc[indexing.linear(i)] = cxx::invoke(
          f, fun::getIndex(xs.data, xs.indexing.linear(i)), args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  struct fmap2 {};

  template <typename F, typename T1, typename T2, typename... Args>
  grid(fmap2, F &&f, const grid<C, T1, D> &xs, const grid<C, T2, D> &ys,
       Args &&... args)
      : indexing(xs.shape()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, Args...>, T>::value,
                  "");
    cxx_assert(ys.shape() == xs.shape());
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    indexing.loop([&](const index_type &i) {
      acc[indexing.linear(i)] =
          cxx::invoke(f, fun::getIndex(xs.data, xs.indexing.linear(i)),
                      fun::getIndex(ys.data, ys.indexing.linear(i)), args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  struct fmap3 {};

  template <typename F, typename T1, typename T2, typename T3, typename... Args>
  grid(fmap3, F &&f, const grid<C, T1, D> &xs, const grid<C, T2, D> &ys,
       const grid<C, T3, D> &zs, Args &&... args)
      : indexing(xs.shape()) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, T2, T3, Args...>, T>::value, "");
    cxx_assert(ys.shape() == xs.shape());
    cxx_assert(zs.shape() == xs.shape());
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    indexing.loop([&](const index_type &i) {
      acc[indexing.linear(i)] =
          cxx::invoke(f, fun::getIndex(xs.data, xs.indexing.linear(i)),
                      fun::getIndex(ys.data, ys.indexing.linear(i)),
                      fun::getIndex(zs.data, zs.indexing.linear(i)), args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // boundary

  struct boundary {};

  grid(boundary, const grid<C, T, D + 1> &xs, std::ptrdiff_t i)
      : indexing(typename index_space::boundary(), xs.indexing, i),
        data(xs.data) {
    cxx_assert(invariant());
  }

  // fmapStencilMulti

  struct fmapStencilMulti {};

  template <typename F, typename G, typename T1, typename... Args>
  grid(fmapStencilMulti, F &&f, G &&g, const grid<C, T1, 0> &xs,
       std::size_t bmask, Args &&... args)
      : indexing(xs.shape()) {
    static_assert(D == 0, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B
        __attribute__((__unused__));
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, std::size_t, Args...>, T>::value,
        "");
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    indexing.loop([&](const index_type &i) {
      std::size_t bdirs = 0;
      acc[indexing.linear(i)] = cxx::invoke(
          f, fun::getIndex(xs.data, xs.indexing.linear(i)), bdirs, args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  template <
      typename F, typename G, typename T1, typename... Args,
      typename BC = grid<C, adt::dummy, 0>,
      typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
      typename BCB = typename fun::fun_traits<BC>::template constructor<B>>
  grid(fmapStencilMulti, F &&f, G &&g, const grid<C, T1, 1> &xs,
       std::size_t bmask, const BCB &bm0, const BCB &bp0, Args &&... args)
      : indexing(xs.shape()) {
    static_assert(D == 1, "");
    typedef cxx::invoke_of_t<F, T1, std::size_t, B, B, Args...> R;
    static_assert(std::is_same<R, T>::value, "");
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    auto di = xs.indexing.linear(array_dir<std::ptrdiff_t, D, 0>());
    indexing.loop([&](const index_type &i) {
      bool isbm0 = i[0] == 0;
      bool isbp0 = i[0] == xs.indexing.shape()[0] - 1;
      std::size_t bdirs = bmask & ((isbm0 << 0) | (isbp0 << 1));
      const B &cm0 =
          isbm0 ? fun::getIndex(bm0.data, bm0.indexing.linear(rmdir<0>(i)))
                : cxx::invoke(
                      g, fun::getIndex(xs.data, xs.indexing.linear(i - di)), 1);
      const B &cp0 =
          isbp0 ? fun::getIndex(bp0.data, bp0.indexing.linear(rmdir<0>(i)))
                : cxx::invoke(
                      g, fun::getIndex(xs.data, xs.indexing.linear(i + di)), 0);
      acc[indexing.linear(i)] =
          cxx::invoke(f, fun::getIndex(xs.data, xs.indexing.linear(i)), bdirs,
                      cm0, cp0, args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  template <
      typename F, typename G, typename T1, typename... Args,
      typename BC = grid<C, adt::dummy, 1>,
      typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
      typename BCB = typename fun::fun_traits<BC>::template constructor<B>>
  grid(fmapStencilMulti, F &&f, G &&g, const grid<C, T1, 2> &xs,
       std::size_t bmask, const BCB &bm0, const BCB &bm1, const BCB &bp0,
       const BCB &bp1, Args &&... args)
      : indexing(xs.shape()) {
    static_assert(D == 2, "");
    typedef cxx::invoke_of_t<F, T1, std::size_t, B, B, B, B, Args...> R;
    static_assert(std::is_same<R, T>::value, "");
    fun::accumulator<container_constructor<T>> acc(indexing.size());
    auto di0 = array_dir<std::ptrdiff_t, D, 0>();
    auto di1 = array_dir<std::ptrdiff_t, D, 1>();
    indexing.loop([&](const index_type &i) {
      bool isbm0 = i[0] == 0;
      bool isbm1 = i[1] == 0;
      bool isbp0 = i[0] == xs.indexing.shape()[0] - 1;
      bool isbp1 = i[1] == xs.indexing.shape()[1] - 1;
      std::size_t bdirs =
          bmask & ((isbm0 << 0) | (isbm1 << 2) | (isbp0 << 1) | (isbp1 << 3));
      const B &cm0 =
          isbm0
              ? fun::getIndex(bm0.data, bm0.indexing.linear(rmdir<0>(i)))
              : cxx::invoke(
                    g, fun::getIndex(xs.data, xs.indexing.linear(i - di0)), 1);
      const B &cm1 =
          isbm1
              ? fun::getIndex(bm1.data, bm1.indexing.linear(rmdir<1>(i)))
              : cxx::invoke(
                    g, fun::getIndex(xs.data, xs.indexing.linear(i - di1)), 3);
      const B &cp0 =
          isbp0
              ? fun::getIndex(bp0.data, bp0.indexing.linear(rmdir<0>(i)))
              : cxx::invoke(
                    g, fun::getIndex(xs.data, xs.indexing.linear(i + di0)), 0);
      const B &cp1 =
          isbp1
              ? fun::getIndex(bp1.data, bp1.indexing.linear(rmdir<1>(i)))
              : cxx::invoke(
                    g, fun::getIndex(xs.data, xs.indexing.linear(i + di1)), 2);
      acc[indexing.linear(i)] =
          cxx::invoke(f, fun::getIndex(xs.data, xs.indexing.linear(i)), bdirs,
                      cm0, cm1, cp0, cp1, args...);
    });
    data = acc.finalize();
    cxx_assert(invariant());
  }

  // foldMap

  template <typename F, typename Op, typename Z, typename... Args,
            typename R = cxx::invoke_of_t<F, T, Args...>>
  R foldMap(F &&f, Op &&op, const Z &z, Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    R r(z);
    indexing.loop([&](const index_type &i) {
      r = cxx::invoke(
          op, std::move(r),
          cxx::invoke(f, fun::getIndex(data, indexing.linear(i)), args...));
    });
    return r;
  }

  template <typename F, typename Op, typename Z, typename T2, typename... Args,
            typename R = cxx::invoke_of_t<F, T, T2, Args...>>
  R foldMap2(F &&f, Op &&op, const Z &z, const grid<C, T2, D> &ys,
             Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    cxx_assert(ys.shape() == shape());
    R r(z);
    indexing.loop([&](const index_type &i) {
      r = cxx::invoke(op, std::move(r),
                      cxx::invoke(f, fun::getIndex(data, indexing.linear(i)),
                                  fun::getIndex(ys.data, ys.indexing.linear(i)),
                                  args...));
    });
    return r;
  }

  // dump
  fun::ostreamer dump() const {
    std::ostringstream os;
    os << "grid{";
    indexing.loop([&](const index_type &i) {
      for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
        if (i[d] > 0)
          break;
        os << "[";
      }
      os << fun::getIndex(data, indexing.linear(i)) << ",";
      for (std::ptrdiff_t d = 0; d < std::ptrdiff_t(D); ++d) {
        if (i[d] < indexing.shape()[d] - 1)
          break;
        os << "],";
      }
    });
    os << "}";
    return fun::ostreamer(os.str());
  }
};
template <typename C, typename T, std::size_t D>
void swap(grid<C, T, D> &x, grid<C, T, D> &y) {
  x.swap(y);
}
} // namespace adt

#define ADT_GRID_IMPL_HPP_DONE
#endif // #ifdef ADT_GRID_IMPL_HPP
#ifndef ADT_GRID_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
