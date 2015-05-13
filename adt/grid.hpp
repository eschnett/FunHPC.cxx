#ifndef ADT_GRID_HPP
#define ADT_GRID_HPP

#include <cxx/invoke.hpp>
#include <fun/array.hpp>

#include <cereal/access.hpp>

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
  index_type m_stride;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(m_shape, m_stride);
  }

  static constexpr index_type origin() noexcept {
    return adt::array_zero<std::ptrdiff_t, D>();
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

public:
  bool invariant() const {
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      if (m_shape[d] < 0)
        return false;
      if (m_stride[d] < 0)
        return false;
    }
    return true;
  }

  index_space(const index_type &shape)
      : m_shape(shape), m_stride(make_stride(m_shape)) {}
  index_space() : index_space(origin()) {}

  const index_type &shape() const { return m_shape; }
  std::size_t size() const { return D == 0 ? 1 : m_stride[D - 1]; }
  bool empty() const { return size() == 0; }

  // Convert a position to a linear index
  std::ptrdiff_t linear(const index_type &i) const {
    std::ptrdiff_t lin = 0;
    std::ptrdiff_t str = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      assert(i[d] >= 0 && i[d] < m_shape[d]);
      lin += i[d] * str;
      str = m_stride[d];
      assert(lin < str);
    }
    return lin;
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
};
}

template <template <typename> class C, typename T, std::ptrdiff_t D>
class grid {
public:
  typedef T value_type;
  static constexpr std::ptrdiff_t rank = D;
  typedef typename detail::index_space<D>::index_type index_type;
  template <typename U> using storage_constructor = C<U>;

  template <template <typename> class C1, typename T1, std::ptrdiff_t D1>
  friend class grid;

private:
  detail::index_space<D> indexing;
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

  bool invariant() const { return indexing.size() == data.size(); }

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

  // fmapTopo

  struct fmapTopo {};

  template <typename F, typename G, typename T1, typename... Args>
  grid(fmapTopo, F &&f, G &&g, const grid<C, T1, 0> &xs, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 0, "");
    // typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(std::is_same<cxx::invoke_of_t<F, T1, Args...>, T>::value, "");
    indexing.loop([&](const index_type &i) {
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)], args...);
    });
    assert(invariant());
  }

  template <typename F, typename G, typename T1, typename BM0, typename BP0,
            typename... Args>
  grid(fmapTopo, F &&f, G &&g, const grid<C, T1, 1> &xs, BM0 &&bm0, BP0 &&bp0,
       Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 1, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, B, B, Args...>, T>::value, "");
    static_assert(std::is_same<std::decay_t<BM0>, grid<C, B, 0>>::value, "");
    static_assert(std::is_same<std::decay_t<BP0>, grid<C, B, 0>>::value, "");
    auto di = xs.indexing.linear(array_dir<std::ptrdiff_t, D, 0>());
    indexing.loop([&](const index_type &i) {
      const B &cm0 =
          i[0] == 0 ? bm0.data[bm0.indexing.linear(rmdir<0>(i))]
                    : cxx::invoke(g, xs.data[xs.indexing.linear(i - di)], 1);
      const B &cp0 =
          i[0] == xs.indexing.shape()[0] - 1
              ? bp0.data[bp0.indexing.linear(rmdir<0>(i))]
              : cxx::invoke(g, xs.data[xs.indexing.linear(i + di)], 0);
      data[indexing.linear(i)] =
          cxx::invoke(f, xs.data[xs.indexing.linear(i)], cm0, cp0, args...);
    });
    assert(invariant());
  }

  template <typename F, typename G, typename T1, typename BM0, typename BP0,
            typename BM1, typename BP1, typename... Args>
  grid(fmapTopo, F &&f, G &&g, const grid<C, T1, 2> &xs, BM0 &&bm0, BP0 &&bp0,
       BM1 &&bm1, BP1 &&bp1, Args &&... args)
      : indexing(xs.shape()), data(indexing.size()) {
    static_assert(D == 2, "");
    typedef cxx::invoke_of_t<G, T1, std::ptrdiff_t> B;
    static_assert(
        std::is_same<cxx::invoke_of_t<F, T1, B, B, B, B, Args...>, T>::value,
        "");
    static_assert(std::is_same<std::decay_t<BM0>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BP0>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BM1>, grid<C, B, 1>>::value, "");
    static_assert(std::is_same<std::decay_t<BP1>, grid<C, B, 1>>::value, "");
    auto di0 = array_dir<std::ptrdiff_t, D, 0>();
    auto di1 = array_dir<std::ptrdiff_t, D, 1>();
    indexing.loop([&](const index_type &i) {
      const B &cm0 =
          i[0] == 0 ? bm0.data[bm0.indexing.linear(rmdir<0>(i))]
                    : cxx::invoke(g, xs.data[xs.indexing.linear(i - di0)], 1);
      const B &cp0 =
          i[0] == xs.indexing.shape()[0] - 1
              ? bp0.data[bp0.indexing.linear(rmdir<0>(i))]
              : cxx::invoke(g, xs.data[xs.indexing.linear(i + di0)], 0);
      const B &cm1 =
          i[1] == 0 ? bm1.data[bm1.indexing.linear(rmdir<1>(i))]
                    : cxx::invoke(g, xs.data[xs.indexing.linear(i - di1)], 3);
      const B &cp1 =
          i[1] == xs.indexing.shape()[1] - 1
              ? bp1.data[bp1.indexing.linear(rmdir<1>(i))]
              : cxx::invoke(g, xs.data[xs.indexing.linear(i + di1)], 2);
      data[indexing.linear(i)] = cxx::invoke(f, xs.data[xs.indexing.linear(i)],
                                             cm0, cp0, cm1, cp1, args...);
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
};
}

#define ADT_GRID_HPP_DONE
#endif // #ifdef ADT_GRID_HPP
#ifndef ADT_GRID_HPP_DONE
#error "Cyclic include dependency"
#endif
