#ifndef ADT_GRID_HPP
#define ADT_GRID_HPP

#include <cxx/invoke.hpp>
#include <fun/array.hpp>

#include <cereal/access.hpp>

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>

namespace adt {

namespace detail {

template <std::ptrdiff_t D> class index_space {
public:
  typedef std::array<std::ptrdiff_t, D> index_type;

private:
  index_type m_shape;
  std::ptrdiff_t m_offset;
  index_type m_stride;
  std::ptrdiff_t m_allocated;

  template <std::ptrdiff_t D1> friend class index_space;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(m_shape, m_offset, m_stride, m_allocated);
  }

  static constexpr index_type origin() noexcept {
    return adt::array_zero<std::ptrdiff_t, D>();
  }

  static std::ptrdiff_t make_offset(const index_type &stride,
                                    const index_type &offset) {
    std::ptrdiff_t off = 0;
    std::ptrdiff_t str = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      assert(offset[d] >= 0 &&
             (stride[d] == 0 ? offset[d] == 0 : offset[d] < stride[d]));
      off += offset[d] * str;
      str = stride[d];
    }
    return off;
  }

  // Note: The stride for dimension 0 is always 1, and is not stored.
  // Instead, we store the total number of points.
  static index_type make_stride(const index_type &shape) {
    index_type stride;
    std::ptrdiff_t i = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      i *= shape[d];
      stride[d] = i;
    }
    assert(D == 0 || stride[D - 1] == prod(shape));
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
    if (adt::any(adt::lt(m_stride, 0)))
      return false;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      if (stride(d) < 0)
        return false;
      if (stride(d) > 0 && stride(d + 1) % stride(d) != 0)
        return false;
    }
    if (m_allocated < 0)
      return false;
    if (m_offset + size() > m_allocated)
      return false;
    auto alloc = allocated();
    if (adt::any(adt::lt(alloc, 0)))
      return false;
    if (adt::any(adt::gt(off + m_shape, alloc)))
      return false;
    return true;
  }

  index_space() : index_space(origin()) {}
  index_space(const index_type &shape) : index_space(shape, origin(), shape) {}
  index_space(const index_type &shape, const index_type &offset,
              const index_type &allocated)
      : m_shape(shape), m_offset(make_offset(make_stride(allocated), offset)),
        m_stride(make_stride(allocated)),
        m_allocated(make_allocated(allocated)) {
    assert(invariant());
  }
  index_space(const index_type &shape, std::ptrdiff_t offset,
              const index_type &stride, std::ptrdiff_t allocated)
      : m_shape(shape), m_offset(offset), m_stride(stride),
        m_allocated(allocated) {
    assert(invariant());
  }

  const index_type &shape() const { return m_shape; }
  std::size_t size() const { return adt::prod(shape()); }
  bool empty() const { return size() == 0; }

  std::ptrdiff_t stride(std::ptrdiff_t d) const {
    assert(d >= 0 && d <= D);
    if (d == 0)
      return D == 0 ? 1 : std::min(std::ptrdiff_t(1), m_stride[0]);
    return m_stride[d - 1];
  }
  index_type offset() const {
    index_type r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r[d] = stride(d) == 0 || stride(d + 1) == 0 ? 0 : (m_offset / stride(d)) %
                                                            stride(d + 1);
    return r;
  }
  index_type allocated() const {
    index_type r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r[d] = stride(d) == 0 ? 0 : stride(d + 1) / stride(d);
    return r;
  }
  std::ptrdiff_t allocated_size() const { return m_allocated; }

  // Convert a position to a linear index
  std::ptrdiff_t linear(const index_type &i) const {
    std::ptrdiff_t lin = m_offset;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      assert(i[d] >= 0 && i[d] < m_shape[d]);
      lin += i[d] * stride(d);
    }
    assert(lin < m_allocated);
    return lin;
  }

  struct boundary {};
  index_space(boundary, const index_space<D + 1> &old, std::ptrdiff_t i) {
    assert(i >= 0 && i < 2 * (D + 1));
    auto f = i % 2, d = i / 2;
    auto new_offset = old.offset();
    new_offset[d] = !f ? 0 : old.shape()[d] - 1;
    auto new_shape = old.shape();
    new_shape[d] = 1;
    auto new_space = index_space<D + 1>(new_shape, new_offset, old.allocated());

    m_shape = adt::rmdir(new_space.m_shape, d);
    m_offset = new_space.m_offset;
    m_stride = adt::rmdir(new_space.m_stride, d);
    m_allocated = new_space.m_allocated;
    assert(invariant());
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
    std::array<std::ptrdiff_t, D + 1> strides;
    for (std::ptrdiff_t d = 0; d <= D; ++d)
      strides[d] = is.stride(d);
    return os << "index_space{m_shape=" << is.m_shape
              << ",m_offset=" << is.m_offset << ",m_stride=" << is.m_stride
              << ",m_allocated=" << is.m_allocated << ";stride()=" << strides
              << ",offset()=" << is.offset() << ",allocated=" << is.allocated()
              << "}";
  }
};
}

template <template <typename> class C, typename T, std::ptrdiff_t D>
class grid {
public:
  typedef T value_type;
  static constexpr std::ptrdiff_t rank = D;
  typedef detail::index_space<D> index_space;
  typedef typename index_space::index_type index_type;
  template <typename U> using storage_constructor = C<U>;

  template <template <typename> class C1, typename T1, std::ptrdiff_t D1>
  friend class grid;

private:
  index_space indexing;
  C<T> data;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    // TODO: Serialize only the accessible part of data
    ar(indexing, data);
  }

public:
  bool empty() const { return indexing.empty(); }
  std::size_t size() const { return indexing.size(); }
  std::array<std::ptrdiff_t, D> shape() const { return indexing.shape(); }

private:
  bool invariant0() const { return indexing.allocated_size() == data.size(); }

public:
  bool invariant() const {
    auto inv = invariant0();
    if (!inv)
      std::cout << "grid::invariant: indexing=" << indexing
                << ", data.size()=" << data.size() << "\n";
    return inv;
  }

  grid() : indexing(), data(indexing.size()) { assert(invariant()); }

  grid(const index_type &shape, const C<T> &data)
      : indexing(shape), data(data) {
    assert(invariant());
  }
  grid(const index_type &shape, C<T> &&data)
      : indexing(shape), data(std::move(data)) {
    assert(invariant());
  }

  grid(const grid &) = default;
  grid(grid &&) = default;
  grid &operator=(const grid &) = default;
  grid &operator=(grid &&) = default;

  const T &head() const {
    assert(!empty());
    return data.front();
  }
  const T &last() const {
    assert(!empty());
    return data.back();
  }

  // iotaMap

  struct iotaMap {};

  template <typename F, typename... Args, std::ptrdiff_t D2 = D,
            typename std::enable_if_t<D2 == 0> * = nullptr>
  grid(iotaMap, F &&f, std::ptrdiff_t s, Args &&... args)
      : indexing(index_type()),
        data(fun::iotaMap<C>(std::forward<F>(f), s,
                             std::forward<Args>(args)...)) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
    assert(invariant());
  }

  template <typename F, typename... Args, std::ptrdiff_t D2 = D,
            typename std::enable_if_t<D2 == 1> * = nullptr>
  grid(iotaMap, F &&f, std::ptrdiff_t s, Args &&... args)
      : indexing(index_type{{s}}),
        data(fun::iotaMap<C>(std::forward<F>(f), s,
                             std::forward<Args>(args)...)) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, std::ptrdiff_t, Args...>, T>::value,
        "");
    assert(invariant());
  }

  // TODO: support generic indexable types, not just std::vector.
  // Probably should define an API, maybe supporting constructing and
  // indexing. A type consisting of "shared pointer to C-style array"
  // might be interesting.
  template <typename F, typename... Args>
  grid(iotaMap, F &&f, const index_type &s, Args &&... args)
      : indexing(s), data(indexing.size()) {
    static_assert(
        std::is_same<cxx::invoke_of_t<F, index_type, Args...>, T>::value, "");
    indexing.loop([&](const index_type &i) {
      data[indexing.linear(i)] = cxx::invoke(f, i, args...);
    });
    assert(invariant());
  }

  // fmap

  struct fmap {};

  template <typename F, typename T1, typename... Args>
  grid(fmap, F &&f, const grid<C, T1, D> &xs, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
    indexing.loop([&](const index_type &i) {
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)], args...);
    });
    assert(invariant());
  }

  struct fmap2 {};

  template <typename F, typename T1, typename T2, typename... Args>
  grid(fmap2, F &&f, const grid<C, T1, D> &xs, const grid<C, T2, D> &ys,
       Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, T2, Args...>, T>::value,
                  "");
    assert(ys.shape() == xs.shape());
    indexing.loop([&](const index_type &i) {
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)],
                      ys.data[ys.indexing.linear(i)], args...);
    });
    assert(invariant());
  }

  // boundary

  struct boundary {};

  grid(boundary, const grid<C, T, D + 1> &xs, std::ptrdiff_t i)
      : indexing(typename index_space::boundary(), xs.indexing, i),
        data(xs.data) {
    assert(invariant());
  }

  // fmapStencil

  struct fmapStencil {};

  template <typename F, typename G, typename T1, typename... Args>
  grid(fmapStencil, F &&f, G &&g, const grid<C, T1, 0> &xs, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 0, "");
    // typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, std::size_t, Args...>, T>::value,
        "");
    indexing.loop([&](const index_type &i) {
      std::size_t bdirs = 0;
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)], bdirs, args...);
    });
    assert(invariant());
  }

  template <typename F, typename G, typename T1, typename BM0, typename BP0,
            typename... Args>
  grid(fmapStencil, F &&f, G &&g, const grid<C, T1, 1> &xs, BM0 &&bm0,
       BP0 &&bp0, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 1, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, std::size_t, B, B, Args...>,
                     T>::value,
        "");
    static_assert(std::is_same<std::decay_t<BM0>, grid<C, B, 0>>::value, "");
    static_assert(std::is_same<std::decay_t<BP0>, grid<C, B, 0>>::value, "");
    auto di = xs.indexing.linear(array_dir<std::ptrdiff_t, D, 0>());
    indexing.loop([&](const index_type &i) {
      bool isbm0 = i[0] == 0;
      bool isbp0 = i[0] == xs.indexing.shape()[0] - 1;
      std::size_t bdirs = (isbm0 << 0) | (isbp0 << 1);
      const B &cm0 =
          isbm0 ? bm0.data[bm0.indexing.linear(rmdir<0>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i - di)], 1);
      const B &cp0 =
          isbp0 ? bp0.data[bp0.indexing.linear(rmdir<0>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i + di)], 0);
      data[indexing.linear(i)] = cxx::invoke(f, xs.data[xs.indexing.linear(i)],
                                             bdirs, cm0, cp0, args...);
    });
    assert(invariant());
  }

  template <typename F, typename G, typename T1, typename BM0, typename BM1,
            typename BP0, typename BP1, typename... Args>
  grid(fmapStencil, F &&f, G &&g, const grid<C, T1, 2> &xs, BM0 &&bm0,
       BM1 &&bm1, BP0 &&bp0, BP1 &&bp1, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 2, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, std::size_t, B, B, B, B, Args...>,
                     T>::value,
        "");
    static_assert(std::is_same<std::decay_t<BM0>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BM1>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BP0>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BP1>, grid<C, B, 1>>::value, "");
    auto di0 = array_dir<std::ptrdiff_t, D, 0>();
    auto di1 = array_dir<std::ptrdiff_t, D, 1>();
    indexing.loop([&](const index_type &i) {
      bool isbm0 = i[0] == 0;
      bool isbm1 = i[1] == 0;
      bool isbp0 = i[0] == xs.indexing.shape()[0] - 1;
      bool isbp1 = i[1] == xs.indexing.shape()[1] - 1;
      std::size_t bdirs =
          (isbm0 << 0) | (isbm1 << 2) | (isbp0 << 1) | (isbp1 << 3);
      const B &cm0 =
          isbm0 ? bm0.data[bm0.indexing.linear(rmdir<0>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i - di0)], 1);
      const B &cm1 =
          isbm1 ? bm1.data[bm1.indexing.linear(rmdir<1>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i - di1)], 3);
      const B &cp0 =
          isbp0 ? bp0.data[bp0.indexing.linear(rmdir<0>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i + di0)], 0);
      const B &cp1 =
          isbp1 ? bp1.data[bp1.indexing.linear(rmdir<1>(i))]
                : cxx::invoke(g, xs.data[xs.indexing.linear(i + di1)], 2);
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)], bdirs, cm0, cm1, cp0,
                      cp1, args...);
    });
    assert(invariant());
  }

  // foldMap

  template <typename F, typename Op, typename Z, typename... Args,
            typename R = cxx::invoke_of_t<F, T, Args...>>
  R foldMap(F &&f, Op &&op, const Z &z, Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    R r(z);
    indexing.loop([&](const index_type &i) {
      r = cxx::invoke(op, std::move(r),
                      cxx::invoke(f, data[indexing.linear(i)], args...));
    });
    return r;
  }

  template <typename F, typename Op, typename Z, typename T2, typename... Args,
            typename R = cxx::invoke_of_t<F, T, T2, Args...>>
  R foldMap2(F &&f, Op &&op, const Z &z, const grid<C, T2, D> &ys,
             Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    assert(ys.shape() == shape());
    R r(z);
    indexing.loop([&](const index_type &i) {
      r = cxx::invoke(op, std::move(r),
                      cxx::invoke(f, data[indexing.linear(i)],
                                  ys.data[ys.indexing.linear(i)], args...));
    });
    return r;
  }
};
}

#define ADT_GRID_HPP_DONE
#endif // #ifdef ADT_GRID_HPP
#ifndef ADT_GRID_HPP_DONE
#error "Cyclic include dependency"
#endif
