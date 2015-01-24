#ifndef CXX_SHAPE_HH
#define CXX_SHAPE_HH

#include "cxx_shape_fwd.hh"

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_utils.hh"

#include <cereal/access.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/tuple.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <ostream>
#include <string>
#include <tuple>

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
  static constexpr vect one() {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = 1;
    return r;
  }
  static constexpr vect iota() {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = d;
    return r;
  }
  static constexpr vect set1(T a) {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = a;
    return r;
  }
  static constexpr vect dir(std::ptrdiff_t d) { return zero().set(d, 1); }
  // Element access
  T operator[](std::ptrdiff_t d) const {
    assert(d >= 0 && d < D);
    return elts[d];
  }
  // Updating
  vect set(std::ptrdiff_t d, const T &a) const {
    assert(d >= 0 && d < D);
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
  vect &operator*=(const vect &i) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] *= i.elts[d];
    return *this;
  }
  vect &operator/=(const vect &i) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] /= i.elts[d];
    return *this;
  }
  vect &operator%=(const vect &i) {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      elts[d] %= i.elts[d];
    return *this;
  }
  vect operator+(const vect &i) const { return vect(*this) += i; }
  vect operator-(const vect &i) const { return vect(*this) -= i; }
  vect operator*(const vect &i) const { return vect(*this) *= i; }
  vect operator/(const vect &i) const { return vect(*this) /= i; }
  vect operator%(const vect &i) const { return vect(*this) %= i; }
  vect div_exact(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::div_exact(elts[d], i.elts[d]);
    return r;
  }
  vect div_floor(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::div_floor(elts[d], i.elts[d]);
    return r;
  }
  vect mod_floor(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::mod_floor(elts[d], i.elts[d]);
    return r;
  }
  vect align_floor(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::align_floor(elts[d], i.elts[d]);
    return r;
  }
  vect div_ceil(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::div_ceil(elts[d], i.elts[d]);
    return r;
  }
  vect mod_ceil(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::mod_ceil(elts[d], i.elts[d]);
    return r;
  }
  vect align_ceil(const vect &i) const {
    vect r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::align_ceil(elts[d], i.elts[d]);
    return r;
  }
  vect &operator*=(const T &a) { return operator*=(vect::set1(a)); }
  vect &operator/=(const T &a) { return operator/=(vect::set1(a)); }
  vect &operator%=(const T &a) { return operator%=(vect::set1(a)); }
  vect operator*(const T &a) const { return operator*(vect::set1(a)); }
  vect operator/(const T &a) const { return operator/(vect::set1(a)); }
  vect operator%(const T &a) const { return operator%(vect::set1(a)); }
  vect div_exact(const T &a) const { return div_exact(vect::set1(a)); }
  vect div_floor(const T &a) const { return div_floor(vect::set1(a)); }
  vect mod_floor(const T &a) const { return mod_floor(vect::set1(a)); }
  vect align_floor(const T &a) const { return align_floor(vect::set1(a)); }
  vect div_ceil(const T &a) const { return div_ceil(vect::set1(a)); }
  vect mod_ceil(const T &a) const { return mod_ceil(vect::set1(a)); }
  vect align_ceil(const T &a) const { return align_ceil(vect::set1(a)); }
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
  T dot(const vect &i) const {
    T r = 0;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r += elts[d] * i.elts[d];
    return r;
  }
  std::ptrdiff_t maxdir() const {
    std::ptrdiff_t dir = -1;
    T r = std::numeric_limits<T>::lowest();
    for (std::ptrdiff_t d = 0; d < D; ++d)
      if (elts[d] > r)
        dir = d, r = elts[d];
    return r;
  }
  T maxval() const {
    T r = std::numeric_limits<T>::lowest();
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = std::max(r, elts[d]);
    return r;
  }
  std::ptrdiff_t mindir() const {
    std::ptrdiff_t dir = -1;
    T r = std::numeric_limits<T>::max();
    for (std::ptrdiff_t d = 0; d < D; ++d)
      if (elts[d] < r)
        dir = d, r = elts[d];
    return r;
  }
  T minval() const {
    T r = std::numeric_limits<T>::max();
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = std::min(r, elts[d]);
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
  std::enable_if_t<std::is_same<cxx::invoke_of_t<Op, T, T>, T>::value, T>
  fold(const Op &op, T r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, elts[d]);
    return r;
  }
  template <typename F, typename Op, typename R>
  std::enable_if_t<(std::is_same<cxx::invoke_of_t<F, T>, R>::value &&
                    std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value),
                   R>
  foldMap(const F &f, const Op &op, R r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, cxx::invoke(f, elts[d]));
    return r;
  }
  template <typename Op, typename R>
  std::enable_if_t<std::is_same<cxx::invoke_of_t<Op, R, T>, R>::value, R>
  foldl(const Op &op, R r) const {
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r = cxx::invoke(op, r, elts[d]);
    return r;
  }
  template <typename F> auto fmap(const F &f) const {
    typedef cxx::invoke_of_t<F, T> R;
    vect<R, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::invoke(f, elts[d]);
    return r;
  }
  template <typename F> auto fmap2(const F &f, const vect &i) const {
    typedef cxx::invoke_of_t<F, T, T> R;
    vect<R, D> r;
    for (std::ptrdiff_t d = 0; d < D; ++d)
      r.elts[d] = cxx::invoke(f, elts[d], i.elts[d]);
    return r;
  }
};
template <typename T, std::ptrdiff_t D>
auto div_exact(const vect<T, D> &i, const vect<T, D> &j) {
  return i.div_exact(j);
}
template <typename T, std::ptrdiff_t D>
auto div_floor(const vect<T, D> &i, const vect<T, D> &j) {
  return i.div_floor(j);
}
template <typename T, std::ptrdiff_t D>
auto mod_floor(const vect<T, D> &i, const vect<T, D> &j) {
  return i.mod_floor(j);
}
template <typename T, std::ptrdiff_t D>
auto align_floor(const vect<T, D> &i, const vect<T, D> &j) {
  return i.align_floor(j);
}
template <typename T, std::ptrdiff_t D>
auto div_ceil(const vect<T, D> &i, const vect<T, D> &j) {
  return i.div_ceil(j);
}
template <typename T, std::ptrdiff_t D>
auto mod_ceil(const vect<T, D> &i, const vect<T, D> &j) {
  return i.mod_ceil(j);
}
template <typename T, std::ptrdiff_t D>
auto align_ceil(const vect<T, D> &i, const vect<T, D> &j) {
  return i.align_ceil(j);
}
template <typename T, std::ptrdiff_t D>
auto div_exact(const vect<T, D> &i, const T &a) {
  return i.div_exact(a);
}
template <typename T, std::ptrdiff_t D>
auto div_floor(const vect<T, D> &i, const T &a) {
  return i.div_floor(a);
}
template <typename T, std::ptrdiff_t D>
auto mod_floor(const vect<T, D> &i, const T &a) {
  return i.mod_floor(a);
}
template <typename T, std::ptrdiff_t D>
auto align_floor(const vect<T, D> &i, const T &a) {
  return i.align_floor(a);
}
template <typename T, std::ptrdiff_t D>
auto div_ceil(const vect<T, D> &i, const T &a) {
  return i.div_ceil(a);
}
template <typename T, std::ptrdiff_t D>
auto mod_ceil(const vect<T, D> &i, const T &a) {
  return i.mod_ceil(a);
}
template <typename T, std::ptrdiff_t D>
auto align_ceil(const vect<T, D> &i, const T &a) {
  return i.align_ceil(a);
}
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
template <typename T, std::ptrdiff_t D>
auto dot(const vect<T, D> &i, const vect<T, D> &j) {
  return i.dot(j);
}
template <typename T, std::ptrdiff_t D> auto maxdir(const vect<T, D> &i) {
  return i.maxdir();
}
template <typename T, std::ptrdiff_t D> auto maxval(const vect<T, D> &i) {
  return i.maxval();
}
template <typename T, std::ptrdiff_t D> auto mindir(const vect<T, D> &i) {
  return i.mindir();
}
template <typename T, std::ptrdiff_t D> auto minval(const vect<T, D> &i) {
  return i.minval();
}
template <typename T, std::ptrdiff_t D> auto prod(const vect<T, D> &i) {
  return i.prod();
}
template <typename T, std::ptrdiff_t D> auto sum(const vect<T, D> &i) {
  return i.sum();
}
template <typename Op, typename T, std::ptrdiff_t D>
auto fold(const Op &op, const T &z, const vect<T, D> &i) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, T, T>, T>::value, "");
  return i.fold(op, z);
}
template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D>
auto foldMap(const F &f, const Op &op, const R &z, const vect<T, D> &i) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return i.foldMap(f, op, z);
}
template <typename Op, typename R, typename T, std::ptrdiff_t D>
auto foldl(const Op &op, const R &z, const vect<T, D> &i) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, T>, R>::value, "");
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
// template <std::ptrdiff_t D> using imask = vect<bool, D>;

// Cartesian grid shape

template <std::ptrdiff_t D> class grid_region {
  index<D> imin_, imax_, istep_;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(imin_, imax_, istep_);
  }

public:
  bool invariant() const {
    return all_of(imax_ >= imin_ && istep_ > index<D>::zero() &&
                  (imax_ - imin_) % istep_ == index<D>::zero());
  }
  static constexpr std::ptrdiff_t rank = D;
  // TODO: hide this function
  // grid_region() = delete;
  grid_region()
      : imin_(index<D>::zero()), imax_(index<D>::zero()),
        istep_(index<D>::one()) {
    /*TODO*/ assert(invariant());
  }
  grid_region(const index<D> &imin_, const index<D> &imax_,
              const index<D> &istep_)
      : imin_(imin_), imax_(imax_), istep_(istep_) {}
  grid_region(const index<D> &imin_, const index<D> &imax_)
      : grid_region(imin_, imax_, index<D>::one()) {}
  grid_region(const index<D> &imax_) : grid_region(index<D>::zero(), imax_) {}
  index<D> imin() const { return imin_; }
  index<D> imax() const { return imax_; }
  index<D> istep() const { return istep_; }
  index<D> shape() const { return (imax_ - imin_) / istep_; }
  bool empty() const { return any_of(imax_ <= imin_); }
  std::ptrdiff_t size() const { return prod(shape()); }
  bool operator==(const grid_region &r) const {
    return all_of(imin_ == r.imin_ && imax_ == r.imax_ && istep_ == r.istep_);
  }
  bool operator!=(const grid_region &r) const { return !(*this == r); }
  bool is_compatible_with(const grid_region &r) const {
    return all_of(istep_ == r.istep_ &&
                  (imin_ - r.imin_) % istep_ == index<D>::zero());
  }
  bool is_subregion_of(const grid_region &r) const {
    assert(is_compatible_with(r));
    return empty() || all_of(imin_ >= r.imin_ && imax_ <= r.imax_);
  }
  index<D> strides() const {
    index<D> r;
    std::ptrdiff_t str = 1;
    for (std::ptrdiff_t d = 0; d < D; ++d) {
      r = r.set(d, str);
      str *= shape()[d];
    }
    return r;
  }
  std::ptrdiff_t linear_max() const {
    /*TODO*/ assert(all_of(shape() > index<D>::zero()));
    return dot(strides(), shape() - index<D>::one()) + 1;
  }
  std::ptrdiff_t linear(const index<D> &i) const {
    /*TODO*/ assert(all_of(i >= imin_ && i < imax_));
    return dot(strides(), div_exact(i - imin_, istep_));
  }
  grid_region boundary(std::ptrdiff_t dir, bool face) const {
    return grid_region(
        { imin_.set(dir, !face ? imin_[dir] : imax_[dir] - istep_[dir]),
          imax_.set(dir, !face ? imin_[dir] + istep_[dir] : imax_[dir]),
          istep_ });
  }
};

template <std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const grid_region<D> &r) {
  return os << r.imin() << ":" << r.imax() << ":" << r.istep();
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
  boundaries(const vect<T, D> &bm, const vect<T, D> &bp) {
    for (std::ptrdiff_t dir = 0; dir < D; ++dir)
      bndss_[0][dir] = bm[dir], bndss_[1][dir] = bp[dir];
  }
  T &operator()(std::ptrdiff_t dir, bool face) {
    assert(dir >= 0 && dir < D);
    return bndss_[face][dir];
  }
  const T &operator()(std::ptrdiff_t dir, bool face) const {
    assert(dir >= 0 && dir < D);
    return bndss_[face][dir];
  }
  template <typename F, typename... As>
  static auto iota(const F &f, const As &... as) {
    union bndsu {
      boundaries<T, D> bnds;
      bndsu() {}
      ~bndsu() {
        for (std::ptrdiff_t face = 0; face < 2; ++face)
          for (std::ptrdiff_t dir = 0; dir < D; ++dir)
            bnds.bndss_[face][dir].~T();
      }
    } bnds;
    for (std::ptrdiff_t face = 0; face < 2; ++face)
      for (std::ptrdiff_t dir = 0; dir < D; ++dir)
        new (&bnds.bnds.bndss_[face][dir])
            T(cxx::invoke(f, dir, bool(face), as...));
    return std::move(bnds.bnds);
  }
  template <typename F, typename Op, typename R, typename... As>
  auto foldMap(const F &f, const Op &op, const R &z, const As &... as) const {
    static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
    static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
    R r(z);
    for (std::ptrdiff_t face = 0; face < 2; ++face)
      for (std::ptrdiff_t dir = 0; dir < D; ++dir)
        r = cxx::invoke(op, std::move(r),
                        cxx::invoke(f, bndss_[face][dir], as...));
    return r;
  }
  template <typename F, typename... As>
  auto fmap(const F &f, const As &... as) const {
    // TODO: call iota
    typedef cxx::invoke_of_t<F, T, As...> R;
    union bndsu {
      boundaries<R, D> bnds;
      bndsu() {}
      ~bndsu() {
        for (std::ptrdiff_t face = 0; face < 2; ++face)
          for (std::ptrdiff_t dir = 0; dir < D; ++dir)
            bnds.bndss_[face][dir].~R();
      }
    } bnds;
    for (std::ptrdiff_t face = 0; face < 2; ++face)
      for (std::ptrdiff_t dir = 0; dir < D; ++dir)
        new (&bnds.bnds.bndss_[face][dir])
            R(cxx::invoke(f, bndss_[face][dir], as...));
    return std::move(bnds.bnds);
  }
};

template <typename T> struct is_boundaries : std::false_type {};
template <typename T, std::ptrdiff_t D>
struct is_boundaries<boundaries<T, D> > : std::true_type {};

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

template <template <typename> class C, typename F, typename... As,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, bool, As...>,
          std::enable_if_t<cxx::is_boundaries<C<R> >::value> * = nullptr>
auto iota(const F &f, const As &... as) {
  return C<R>::iota(f, as...);
}

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const boundaries<T, D> &bs,
             const As &... as) {
  return bs.foldMap(f, op, z, as...);
}

template <typename F, typename T, std::ptrdiff_t D, typename... As>
auto fmap(const F &f, const boundaries<T, D> &bs, const As &... as) {
  return bs.fmap(f, as...);
}
}

#define CXX_SHAPE_HH_DONE
#else
#ifndef CXX_SHAPE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_SHAPE_HH
