#ifndef CXX_GRID_HH
#define CXX_GRID_HH

// #include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
// #include "cxx_iota.hh"
#include "cxx_kinds.hh"

#include <cereal/access.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ostream>
#include <tuple>
#include <vector>

namespace cxx {

// Multi-index

template <typename T, std::ptrdiff_t D> class vect {
  std::array<T, D> elts;

  template <typename T1, std::ptrdiff_t D1> friend class vect;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(elts); }

public:
  // Container stuff
  typedef T value_type;
  std::size_t size() const { return D; }
  // Metadata
  static constexpr std::ptrdiff_t rank = D;
  // Constructors
  static constexpr vect zero() {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = 0;
    return r;
  }
  static constexpr vect set1(T a) {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = a;
    return r;
  }
  static constexpr vect iota() {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = d;
    return r;
  }
  static constexpr vect dir(std::ptrdiff_t d) { return zero().set(d, 1); }
  // Element access
  T operator[](std::ptrdiff_t d) const {
    // assert(d >= 0 && d < D);
    return elts[d];
  }
  // Updating
  vect set(std::ptrdiff_t d, const T &a) const {
    // assert(d >= 0 && d < D);
    vect r(*this);
    r.elts[d] = a;
    return r;
  }
  // Conversion
  template <typename R> operator vect<R, D>() const {
    vect<R, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = R(elts[d]);
    return r;
  }
  // Logic
  vect operator!() const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = !elts[d];
    return r;
  }
  vect operator&&(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = elts[d] && i.elts[d];
    return r;
  }
  vect operator||(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = elts[d] || i.elts[d];
    return *r;
  }
  // Vector space
  vect operator+() const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = +elts[d];
    return r;
  }
  vect operator-() const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = -elts[d];
    return r;
  }
  vect &operator+=(const vect &i) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] += i.elts[d];
    return *this;
  }
  vect &operator-=(const vect &i) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] -= i.elts[d];
    return *this;
  }
  vect operator+(const vect &i) const { return vect(*this) += i; }
  vect operator-(const vect &i) const { return vect(*this) -= i; }
  vect &operator*=(const T &a) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] *= a;
    return *this;
  }
  vect operator*(const T &a) const { return vect(*this) *= a; }
  // Comparisons
  vect operator==(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = elts[d] == i.elts[d];
    return r;
  }
  vect operator!=(const vect &i) const { return !(*this == i); }
  vect operator<(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = elts[d] < i.elts[d];
    return r;
  }
  vect operator>(const vect &i) const { return i < *this; }
  vect operator<=(const vect &i) const { return !(*this > i); }
  vect operator>=(const vect &i) const { return i <= *this; }
  vect max(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = std::max(elts[d], i.elts[d]);
    return r;
  }
  vect min(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = std::min(elts[d], i.elts[d]);
    return r;
  }
  template <typename U>
  vect<U, D> ifthen(const vect<U, D> &i, const vect<U, D> &j) const {
    vect<U, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = elts[d] ? i.elts[d] : j.elts[d];
    return r;
  }
  // Reductions
  bool all() const {
    bool r = true;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = r && elts[d];
    return r;
  }
  bool any() const {
    bool r = false;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = r || elts[d];
    return r;
  }
  T prod() const {
    T r = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r *= elts[d];
    return r;
  }
  T sum() const {
    T r = 0;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r += elts[d];
    return r;
  }
  // Prefix reductions
  // (TODO)
  // Higher order
  template <typename Op>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<Op, T, T>::type, T>::value, T>::type
  fold(const Op &op, T r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, elts[d]);
    return r;
  }
  template <typename F, typename Op, typename R>
  typename std::enable_if<
      (std::is_same<typename cxx::invoke_of<F, T>::type, R>::value &&
       std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
      R>::type
  foldMap(const F &f, const Op &op, R r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, cxx::invoke(f, elts[d]));
    return r;
  }
  template <typename Op, typename R>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<Op, R, T>::type, R>::value, R>::type
  foldl(const Op &op, R r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, elts[d]);
    return r;
  }
  template <typename F> auto fmap(const F &f) const {
    typedef typename cxx::invoke_of<F, T>::type R;
    vect<R, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::invoke(f, elts[d]);
    return r;
  }
  template <typename F> auto fmap2(const F &f, const vect &i) const {
    typedef typename cxx::invoke_of<F, T, T>::type R;
    vect<R, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::invoke(f, elts[d], i.elts[d]);
    return r;
  }
};
template <typename T, std::ptrdiff_t D>
auto operator*(const T &a, const vect<T, D> &i) {
  return i * a;
}
template <typename T, std::ptrdiff_t D>
auto max(const vect<T, D> &i, const vect<T, D> &j) {
  return i.max(j);
}
template <typename T, std::ptrdiff_t D>
auto min(const vect<T, D> &i, const vect<T, D> &j) {
  return i.min(j);
}
template <typename T, typename U, std::ptrdiff_t D>
auto ifthen(const vect<T, D> &b, const vect<U, D> &i, const vect<U, D> &j) {
  return b.ifthen(i, j);
}
template <typename T, std::ptrdiff_t D> auto all_of(const vect<T, D> &i) {
  return i.all();
}
template <typename T, std::ptrdiff_t D> auto any_of(const vect<T, D> &i) {
  return i.any();
}
template <typename T, std::ptrdiff_t D> auto prod(const vect<T, D> &i) {
  return i.prod();
}
template <typename T, std::ptrdiff_t D> auto sum(const vect<T, D> &i) {
  return i.sum();
}
template <typename Op, typename T, std::ptrdiff_t D>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, T, T>::type, T>::value, T>::type
fold(const Op &op, const T &z, const vect<T, D> &i) {
  return i.fold(op, z);
}
template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const vect<T, D> &i) {
  return i.foldMap(f, op, z);
}
template <typename Op, typename R, typename T, std::ptrdiff_t D>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, T>::type, R>::value, R>::type
foldl(const Op &op, const R &z, const vect<T, D> &i) {
  return i.foldl(op, z);
}
template <typename F, typename T, std::ptrdiff_t D>
auto fmap(const F &f, const vect<T, D> &i) {
  return i.fmap(f);
}
template <typename F, typename T, std::ptrdiff_t D>
auto fmap2(const F &f, const vect<T, D> &i, const vect<T, D> &j) {
  return i.fmap2(f, j);
}
template <typename T, std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const vect<T, D> &i) {
  os << "[";
  for (std::ptrdiff_t d = 0; d < D; ++d) {
    if (d != 0)
      os << ",";
    os << i[d];
  }
  os << "]";
  return os;
}

template <std::ptrdiff_t D> using index = vect<std::ptrdiff_t, D>;
template <std::ptrdiff_t D> using imask = vect<bool, D>;

// Cartesian grid shape

template <std::ptrdiff_t D> class grid_region {
  index<D> imin_, imax_;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(imin_, imax_); }

public:
  bool invariant() const { return all_of(imax_ >= imin_); }
  static constexpr std::ptrdiff_t rank = D;
  // TODO: hide this function
  // grid_region() = delete;
  grid_region() : imin_(index<D>::zero()), imax_(index<D>::zero()) {}
  grid_region(const index<D> &imin_, const index<D> &imax_)
      : imin_(imin_), imax_(imax_) {}
  grid_region(const index<D> &imax_) : grid_region(index<D>::zero(), imax_) {}
  index<D> imin() const { return imin_; }
  index<D> imax() const { return imax_; }
  index<D> shape() const { return imax_ - imin_; }
  bool empty() const { return any_of(imax_ == imin_); }
  std::ptrdiff_t size() const { return prod(shape()); }
  bool operator==(const grid_region &r) const {
    return all_of(imin_ == r.imin_ && imax_ == r.imax_);
  }
  bool operator!=(const grid_region &r) const { return !(*this == r); }
  bool is_sub_region_of(const grid_region &r) const {
    return all_of(imin_ >= r.imin_ && imax_ <= r.imax_);
  }
  std::ptrdiff_t linear(const index<D> &i) const {
    // assert(all_of(i >= imin_ && i < imax_));
    std::ptrdiff_t r = 0, off = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      r += off * (i[d] - imin_[d]);
      off *= imax_[d] - imin_[d];
    }
    return r;
  }
  grid_region boundary(std::ptrdiff_t dir, bool face) const {
    return grid_region({ imin_.set(dir, !face ? imin_[dir] : imax_[dir] - 1),
                         imax_.set(dir, !face ? imin_[dir] + 1 : imax_[dir]) });
  }
};

template <std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const grid_region<D> &r) {
  return os << r.imin() << ":" << r.imax();
}

template <typename T, std::ptrdiff_t D> class boundaries {
  // std::array has problems with default-construction
  // std::array<std::array<T, D>, 2> bndss_;
  T bndss_[2][D];

  template <typename T1, std::ptrdiff_t D1> friend class boundaries;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    // ar(bndss_);
    for (std::ptrdiff_t face = 0; face < 2; ++face)
      for (std::ptrdiff_t dir = 0; dir < D; ++dir)
        ar(bndss_[face][dir]);
  }

public:
  typedef T value_type;
  static constexpr std::ptrdiff_t rank = D;
  // TODO: Avoid this function, or hide it
  boundaries() {}
  // TODO: Avoid this function, or hide it
  T &operator()(std::ptrdiff_t dir, bool face) {
    assert(dir >= 0 && dir < D);
    return bndss_[face][dir];
  }
  const T &operator()(std::ptrdiff_t dir, bool face) const {
    assert(dir >= 0 && dir < D);
    return bndss_[face][dir];
  }
  struct fmap : std::tuple<> {};
  template <typename F, typename T1, typename... As>
  boundaries(
      typename std::enable_if<
          std::is_same<typename cxx::invoke_of<F, T1, As...>::type, T>::value,
          fmap>::type,
      const F &f, const boundaries<T1, D> &bs, const As &... as)
      : bndss_(cxx::fmap([f, as...](const std::array<T1, D> &bnds) {
                           return cxx::fmap(f, bnds, as...);
                         },
                         bs.bndss_)) {}
};

template <typename T, std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const boundaries<T, D> &bs) {
  os << "[";
  for (std::ptrdiff_t face = 0; face < 2; ++face) {
    if (face != 0)
      os << ",";
    os << "[";
    for (std::ptrdiff_t dir = 0; dir < D; ++dir) {
      if (dir != 0)
        os << ",";
      os << bs(dir, face);
    }
    os << "]";
  }
  os << "]";
  return os;
}

template <typename F, typename T, std::ptrdiff_t D, typename... As>
auto fmap(const F &f, const boundaries<T, D> &bs, const As &... as) {
  typedef typename cxx::invoke_of<F, T, As...>::type R;
  return boundaries<R, D>(typename boundaries<R, D>::fmap(), f, bs, as...);
}

// Cartesian grid

// Helper for foldable
template <typename F, typename Op, std::ptrdiff_t D, typename T, typename T1,
          typename... As>
struct grid_foldMap {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t xstr = xr.linear(index<D>::dir(d));
    T r(z);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r),
                      foldMap<d - 1>(f, op, z, rr, xr, xs + a * xstr, as...));
    return std::move(r);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
// assert(rr.linear(index<D>::dir(d)) == 1);
// assert(xr.linear(index<D>::dir(d)) == 1);
#pragma omp declare reduction(op : T : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
    T r(z);
#pragma omp simd reduction(op : r)
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r),
                      foldMap<d - 1>(f, op, z, rr, xr, xs + a, as...));
    return std::move(r);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), T>::type
  foldMap(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
          const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    return cxx::invoke(f, *xs, as...);
  }
};

template <typename F, typename Op, std::ptrdiff_t D, typename T, typename T1,
          typename T2, typename... As>
struct grid_foldMap2 {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t xstr = xr.linear(index<D>::dir(d));
    std::ptrdiff_t ystr = yr.linear(index<D>::dir(d));
    T r(z);
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(op, std::move(r),
                      foldMap2<d - 1>(f, op, z, rr, xr, xs + a * xstr, yr,
                                      ys + a * ystr, as...));
    return std::move(r);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
// assert(rr.linear(index<D>::dir(d)) == 1);
// assert(xr.linear(index<D>::dir(d)) == 1);
// assert(yr.linear(index<D>::dir(d)) == 1);
#pragma omp declare reduction(op : T : (omp_out = cxx::invoke(                 \
                                            op, std::move(omp_out), omp_in,    \
                                            as...))) initializer(omp_priv(z))
    T r(z);
#pragma omp simd reduction(op : r)
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      r = cxx::invoke(
          op, std::move(r),
          foldMap2<d - 1>(f, op, z, rr, xr, xs + a, yr, ys + a, as...));
    return std::move(r);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), T>::type
  foldMap2(const F &f, const Op &op, const T &z, const grid_region<D> &rr,
           const grid_region<D> &xr, const T1 *restrict xs,
           const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    return cxx::invoke(f, *xs, *ys, as...);
  }
};

// Helper for functor
template <typename F, std::ptrdiff_t D, typename T, typename T1, typename... As>
struct grid_fmap {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.linear(index<D>::dir(d));
    std::ptrdiff_t xstr = xr.linear(index<D>::dir(d));
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap<d - 1>(f, rr, rs + a * rstr, xr, xs + a * xstr, as...);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
// assert(rr.linear(index<D>::dir(d)) == 1);
// assert(xr.linear(index<D>::dir(d)) == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap<d - 1>(f, rr, rs + a, xr, xs + a, as...);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  fmap(const F &f, const grid_region<D> &rr, T *restrict rs,
       const grid_region<D> &xr, const T1 *restrict xs, const As &... as) {
    *rs = cxx::invoke(f, *xs, as...);
  }
};

template <typename F, std::ptrdiff_t D, typename T, typename T1, typename T2,
          typename... As>
struct grid_fmap2 {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.linear(index<D>::dir(d));
    std::ptrdiff_t xstr = xr.linear(index<D>::dir(d));
    std::ptrdiff_t ystr = yr.linear(index<D>::dir(d));
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap2<d - 1>(f, rr, rs + a * rstr, xr, xs + a * xstr, yr, ys + a * ystr,
                   as...);
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
// assert(rr.linear(index<D>::dir(d)) == 1);
// assert(xr.linear(index<D>::dir(d)) == 1);
// assert(yr.linear(index<D>::dir(d)) == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a)
      fmap2<d - 1>(f, rr, rs + a, xr, xs + a, yr, ys + a, as...);
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  fmap2(const F &f, const grid_region<D> &rr, T *restrict rs,
        const grid_region<D> &xr, const T1 *restrict xs,
        const grid_region<D> &yr, const T2 *restrict ys, const As &... as) {
    *rs = cxx::invoke(f, *xs, *ys, as...);
  }
};

template <typename F, typename G, std::ptrdiff_t D, typename T, typename T1,
          typename B, typename... As>
struct grid_stencil_fmap {
  static constexpr std::size_t bit(std::size_t i) {
    return std::size_t(1) << i;
  }
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t dir, std::size_t isouters = 0>
  static typename std::enable_if<(dir > 0 && dir < D), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[dir];
    std::ptrdiff_t rstr = rr.linear(index<D>::dir(dir));
    std::ptrdiff_t xstr = xr.linear(index<D>::dir(dir));
    if (nelts == 1) {
      // both boundaries
      constexpr std::size_t newisouters =
          isouters | (bit(2 * dir + 0) | bit(2 * dir + 1));
      std::ptrdiff_t a = 0;
      auto newbss = bss;
      for (std::ptrdiff_t f = 0; f < 2; ++f)
        for (std::ptrdiff_t d = 0; d < D; ++d)
          if (d != dir)
            newbss(d, f) += a * brs(d, f).linear(index<D>::dir(dir));
      stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                         xs + a * xstr, brs, newbss, as...);
    } else if (nelts > 1) {
      // lower boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 0);
        std::ptrdiff_t a = 0;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).linear(index<D>::dir(dir));
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
      // interior
      for (std::ptrdiff_t a = 1; a < nelts - 1; ++a) {
        constexpr std::size_t newisouters = isouters;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).linear(index<D>::dir(dir));
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
      // upper boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 1);
        std::ptrdiff_t a = nelts - 1;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a * brs(d, f).linear(index<D>::dir(dir));
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a * rstr, xr,
                                           xs + a * xstr, brs, newbss, as...);
      }
    }
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t dir, std::size_t isouters = 0>
  static typename std::enable_if<(dir == 0 && dir < D), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[dir];
    // assert(rr.linear(index<D>::dir(dir)) == 1);
    // assert(xr.linear(index<D>::dir(dir)) == 1);
    // for (std::ptrdiff_t f = 0; f < 2; ++f)
    //   for (std::ptrdiff_t d = 0; d < D; ++d)
    //     assert(brs(d, f).linear(index<D>::dir(dir)) == 1);
    if (nelts == 1) {
      // both boundaries
      constexpr std::size_t newisouters =
          isouters | (bit(2 * dir + 0) | bit(2 * dir + 1));
      std::ptrdiff_t a = 0;
      auto newbss = bss;
      for (std::ptrdiff_t f = 0; f < 2; ++f)
        for (std::ptrdiff_t d = 0; d < D; ++d)
          if (d != dir)
            newbss(d, f) += a;
      stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                         newbss, as...);
    } else if (nelts > 1) {
      // lower boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 0);
        std::ptrdiff_t a = 0;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
// interior
#pragma omp simd
      for (std::ptrdiff_t a = 1; a < nelts - 1; ++a) {
        constexpr std::size_t newisouters = isouters;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
      // upper boundary
      {
        constexpr std::size_t newisouters = isouters | bit(2 * dir + 1);
        std::ptrdiff_t a = nelts - 1;
        auto newbss = bss;
        for (std::ptrdiff_t f = 0; f < 2; ++f)
          for (std::ptrdiff_t d = 0; d < D; ++d)
            if (d != dir)
              newbss(d, f) += a;
        stencil_fmap<dir - 1, newisouters>(f, g, rr, rs + a, xr, xs + a, brs,
                                           newbss, as...);
      }
    }
  }
  // Terminating case for single elements
  template <std::ptrdiff_t dir, std::size_t isouters>
  static typename std::enable_if<(dir == -1), void>::type
  stencil_fmap(const F &f, const G &g, const grid_region<D> &rr, T *restrict rs,
               const grid_region<D> &xr, const T1 *restrict xs,
               const boundaries<grid_region<D>, D> &brs,
               const boundaries<const B * restrict, D> &bss, const As &... as) {
    boundaries<B, D> newbs;
    for (std::ptrdiff_t f = 0; f < 2; ++f) {
      for (std::ptrdiff_t d = 0; d < D; ++d) {
        bool isouter = isouters & bit(2 * d + f);
        auto off = (!f ? -1 : +1) * xr.linear(index<D>::dir(d));
        newbs(d, f) = isouter ? *bss(d, f) : cxx::invoke(g, xs[off], d, f);
      }
    }
    *rs = cxx::invoke(f, *xs, newbs, as...);
  }
};

// Helper for iota
template <typename F, std::ptrdiff_t D, typename T, typename... As>
struct grid_iota {
  // Generic loop, recursing to a lower dimension
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d > 0 && d < D), void>::type
  iota(const F &f, const grid_region<D> &rr, T *restrict rs, index<D> i,
       const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
    std::ptrdiff_t rstr = rr.linear(index<D>::dir(d));
    for (std::ptrdiff_t a = 0; a < nelts; ++a) {
      index<D> j = i + a * index<D>::dir(d);
      iota<d - 1>(f, rr, rs + a * rstr, j, as...);
    }
  }
  // Special case for dir==0 where the stride is known to be 1
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == 0 && d < D), void>::type
  iota(const F &f, const grid_region<D> &rr, T *restrict rs, index<D> i,
       const As &... as) {
    std::ptrdiff_t nelts = rr.shape()[d];
// assert(rr.linear(index<D>::dir(d)) == 1);
#pragma omp simd
    for (std::ptrdiff_t a = 0; a < nelts; ++a) {
      index<D> j = i + a * index<D>::dir(d);
      iota<d - 1>(f, rr, rs + a, j, as...);
    }
  }
  // Terminating case for single elements
  template <std::ptrdiff_t d>
  static typename std::enable_if<(d == -1), void>::type
  iota(const F &f, const grid_region<D> &rr, T *restrict rs, index<D> i,
       const As &... as) {
    *rs = cxx::invoke(f, i, as...);
  }
};

template <typename T, std::ptrdiff_t D> class grid {
  template <typename T1, std::ptrdiff_t D1> friend class grid;

  // Data may be shared with other grids
  std::shared_ptr<std::vector<T> > data_;
  grid_region<D> layout_; // data memory layout
  grid_region<D> region_; // our grid_region

  grid(const grid_region<D> &region)
      : data_(std::make_shared<std::vector<T> >(region.size())),
        layout_(region), region_(region) {}

  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const {
    // Are we referring to all of the data?
    if (region_ == layout_) {
      // Yes: save everything
      // TODO: handle padding
      ar(data_, layout_, region_);
    } else {
      // No: create a copy of the respective sub-grid_region, and save it
      grid(copy(), *this).save(ar);
    }
  }
  template <typename Archive> void load(Archive &ar) {
    ar(data_, layout_, region_);
  }

public:
  bool invariant() const {
    return bool(data_) && layout_.size() == data_->size() &&
           region_.is_sub_region_of(layout_);
  }
  grid_region<D> region() const { return region_; }
  bool empty() const { return region_.empty(); }
  std::ptrdiff_t size() const { return region_.size(); }
  index<D> shape() const { return region_.shape(); }
  //
  // TODO: hide this function
  grid() : grid(grid_region<D>()) {}
  struct copy : std::tuple<> {};
  grid(copy, const grid &g) : grid(fmap(), [](T x) { return x; }, region_, g) {}
  struct sub_region : std::tuple<> {};
  grid(sub_region, const grid &xs, const grid_region<D> &region)
      : data_(xs.data_), layout_(xs.layout_), region_(region) {
    assert(region.is_sub_region_of(xs.region_));
  }
  struct boundary : std::tuple<> {};
  grid(boundary, const grid &xs, std::ptrdiff_t dir, bool face)
      : grid(sub_region(), xs, xs.region_.boundary(dir, face)) {}
  // foldable
  template <typename F, typename Op, typename R, typename... As>
  typename std::enable_if<
      (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
       std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
      R>::type
  foldMap(const F &f, const Op &op, const R &z, const As &... as) const {
    return grid_foldMap<F, Op, D, R, T, As...>::template foldMap<D - 1>(
        f, op, z, region_, layout_,
        data_->data() + layout_.linear(region_.imin() - layout_.imin()), as...);
  }
  template <typename F, typename Op, typename R, typename T1, typename... As>
  typename std::enable_if<
      (std::is_same<typename cxx::invoke_of<F, T, T1, As...>::type, R>::value &&
       std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
      R>::type
  foldMap2(const F &f, const Op &op, const R &z, const grid<T1, D> &ys,
           const As &... as) const {
    assert(region_.is_subregion_of(ys.region_));
    return grid_foldMap2<F, Op, D, R, T, T1, As...>::template foldMap2<D - 1>(
        f, op, z, region_, layout_,
        data_->data() + layout_.linear(region_.imin() - layout_.imin()),
        ys.layout_,
        ys.data_->data() +
            ys.layout_.linear(ys.region_.imin() - ys.layout_.imin()),
        as...);
  }
  // functor
  struct fmap : std::tuple<> {};
  template <typename F, typename T1, typename... As>
  grid(typename std::enable_if<
           std::is_same<typename cxx::invoke_of<F, T1, As...>::type, T>::value,
           fmap>::type,
       const F &f, const grid_region<D> &rr, const grid<T1, D> &xs,
       const As &... as)
      : grid(rr) {
    assert(rr.is_sub_region_of(xs.region_));
    grid_fmap<F, D, T, T1, As...>::template fmap<D - 1>(
        f, layout_, data_->data(), xs.layout_,
        xs.data_->data() +
            xs.layout_.linear(xs.region_.imin() - xs.layout_.imin()),
        as...);
  }
  struct fmap2 : std::tuple<> {};
  template <typename F, typename T1, typename T2, typename... As>
  grid(typename std::enable_if<
           std::is_same<typename cxx::invoke_of<F, T1, T2, As...>::type,
                        T>::value,
           fmap2>::type,
       const F &f, const grid_region<D> &rr, const grid<T1, D> &xs,
       const grid<T2, D> &ys, const As &... as)
      : grid(rr) {
    assert(rr.is_sub_region_of(xs.region_));
    assert(rr.is_sub_region_of(ys.region_));
    grid_fmap2<F, D, T, T1, T2, As...>::template fmap2<D - 1>(
        f, layout_, data_->data(), xs.layout_,
        xs.data_->data() +
            xs.layout_.linear(xs.region_.imin() - xs.layout_.imin()),
        ys.layout_,
        ys.data_->data() +
            ys.layout_.linear(ys.region_.imin() - ys.layout_.imin()),
        as...);
  }
  struct stencil_fmap : std::tuple<> {};
  template <typename F, typename G, typename T1, typename B, typename... As>
  grid(typename std::enable_if<
           (std::is_same<
                typename cxx::invoke_of<F, T1, boundaries<B, D>, As...>::type,
                T>::value &&
            std::is_same<
                typename cxx::invoke_of<G, T1, std::ptrdiff_t, bool>::type,
                B>::value),
           stencil_fmap>::type,
       const F &f, const G &g, const grid_region<D> &rr, const grid<T1, D> &xs,
       const boundaries<grid<B, D>, D> &bs, const As &... as)
      : grid(rr) {
    assert(rr.is_sub_region_of(xs.region_));
#if 0
    boundaries<grid_region<D>, D> brs(
        cxx::fmap([](const grid<B, D> &g) { return g.region_; }, bs));
    boundaries<const B * restrict, D> bss(
        cxx::fmap([](const grid<B, D> &g) -> const B *
                  restrict { return g.data_->data(); },
                  bs));
#endif
    boundaries<grid_region<D>, D> brs;
    boundaries<const B * restrict, D> bss;
    for (std::ptrdiff_t f = 0; f < 2; ++f) {
      for (std::ptrdiff_t d = 0; d < D; ++d) {
        // TODO: also check that indices match
        assert(all_of(bs(d, f).region_.shape() ==
                      xs.region_.boundary(d, f).shape()));
        brs(d, f) = bs(d, f).region_;
        bss(d, f) = bs(d, f).data_->data();
      }
    }
    grid_stencil_fmap<F, G, D, T, T1, B, As...>::template stencil_fmap<D - 1>(
        f, g, layout_, data_->data(), xs.layout_, xs.data_->data(), brs, bss,
        as...);
  }
  // iota
  struct iota : std::tuple<> {};
  template <typename F, typename... As>
  grid(typename std::enable_if<
           std::is_same<typename cxx::invoke_of<F, index<D>, As...>::type,
                        T>::value,
           iota>::type,
       const F &f, const grid_region<D> &rr, const As &... as)
      : grid(rr) {
    grid_iota<F, D, T, As...>::template iota<D - 1>(f, layout_, data_->data(),
                                                    layout_.imin(), as...);
  }
};

// kinds

template <typename T, std::ptrdiff_t D> struct kinds<grid<T, D> > {
  typedef T value_type;
  template <typename U> using constructor = grid<U, D>;
};
template <typename T> struct is_grid : std::false_type {};
template <typename T, std::ptrdiff_t D>
struct is_grid<grid<T, D> > : std::true_type {};

// foldable

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const grid<T, D> &xs,
             const As &... as) {
  return xs.foldMap(f, op, z, as...);
}

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename T2, typename... As>
auto foldMap2(const F &f, const Op &op, const R &z, const grid<T, D> &xs,
              const grid<T2, D> &ys, const As &... as) {
  return xs.foldMap2(f, op, z, ys, as...);
}

// functor

template <typename F, typename T, std::ptrdiff_t D, typename... As,
          typename CT = grid<T, D>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
auto fmap(const F &f, const grid<T, D> &xs, const As &... as) {
  return C<R>(typename C<R>::fmap(), f, xs.region(), xs, as...);
}

template <typename F, typename T, std::ptrdiff_t D, typename T2, typename... As,
          typename CT = grid<T, D>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
auto fmap2(const F &f, const grid<T, D> &xs, const grid<T2, D> &ys,
           const As &... as) {
  return C<R>(typename C<R>::fmap2(), f, xs.region(), xs, ys, as...);
}

template <
    typename F, typename G, typename T, std::ptrdiff_t D, typename B,
    typename... As, typename CT = grid<T, D>,
    template <typename> class C = cxx::kinds<CT>::template constructor,
    typename R = typename cxx::invoke_of<F, T, boundaries<B, D>, As...>::type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<G, T, std::ptrdiff_t, bool>::type,
                 B>::value,
    C<R> >::type
stencil_fmap(const F &f, const G &g, const grid<T, D> &xs,
             const boundaries<grid<B, D>, D> &bs, const As &... as)

{
  return C<R>(typename C<R>::stencil_fmap(), f, g, xs.region(), xs, bs, as...);
}

// iota

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As,
          typename T = typename cxx::invoke_of<F, index<D>, As...>::type>
typename std::enable_if<cxx::is_grid<C<T> >::value, C<T> >::type
iota(const F &f, const grid_region<D> &range, const As &... as) {
  return C<T>(typename C<T>::iota(), f, range, as...);
}

// output

template <typename T, std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const grid<T, D> &g) {
  struct detail {
    template <typename U> using grid_ = grid<U, D>;
  };
  // TODO: This is not efficient
  auto is = cxx::iota<detail::template grid_>([](index<D> i) { return i; },
                                              g.region());
  fmap2([pos = &os](index<D> i, T x) {
          *pos << "  " << i << ": " << x << "\n";
          return std::tuple<>();
        },
        is, g);
  return os;
}
}

#endif // #ifndef CXX_GRID_HH
