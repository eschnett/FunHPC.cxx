#ifndef ADT_INDEX_HPP
#define ADT_INDEX_HPP

#include <adt/arith.hpp>
#include <cxx/cassert.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>

#include <cereal/types/array.hpp>

namespace adt {

// Integer range
class irange_t {
  std::ptrdiff_t imin_, imax_, istep_;
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(imin_, imax_, istep_);
  }

public:
  constexpr bool invariant() const noexcept { return istep_ > 0; }
  constexpr irange_t() : irange_t(0) {}
  constexpr irange_t(std::ptrdiff_t imax_) : irange_t(0, imax_) {}
  constexpr irange_t(std::ptrdiff_t imin_, std::ptrdiff_t imax_)
      : irange_t(imin_, imax_, 1) {}
  constexpr irange_t(std::ptrdiff_t imin_, std::ptrdiff_t imax_,
                     std::ptrdiff_t istep_)
      : imin_(imin_), imax_(imax_), istep_(istep_) {
    cxx_assert(invariant());
  }
  constexpr std::ptrdiff_t imin() const { return imin_; } // rename to head?
  constexpr std::ptrdiff_t imax() const { return imax_; }
  constexpr std::ptrdiff_t istep() const { return istep_; }
  constexpr std::ptrdiff_t shape() const {
    return std::max(std::ptrdiff_t(0),
                    cxx::div_ceil(imax_ - imin_, istep_).quot);
  }
  constexpr std::size_t size() const { return shape(); }
  constexpr bool empty() const { return imax_ <= imin_; }
  constexpr std::ptrdiff_t operator[](std::ptrdiff_t i) const {
    return imin_ + i * istep_;
  }
  friend std::ostream &operator<<(std::ostream &os, const irange_t &inds) {
    return os << "irange_t(" << inds.imin_ << ":" << inds.imax_ << ":"
              << inds.istep_ << ")";
  }
};

// Multi-index
template <std::size_t D> using index_t = std::array<std::ptrdiff_t, D>;

// Multi-dimensional unit range
template <std::size_t D> class range_t {
  index_t<D> imin_, imax_;
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(imin_, imax_); }

public:
  constexpr static index_t<D> zero() { return adt::set<index_t<D>>(0); }
  constexpr static index_t<D> one() { return adt::set<index_t<D>>(1); }
  constexpr bool invariant() const noexcept { return true; }
  constexpr range_t() : range_t(zero()) {}
  constexpr range_t(const index_t<D> &imax_) : range_t(zero(), imax_) {}
  constexpr range_t(const index_t<D> &imin_, const index_t<D> &imax_)
      : imin_(imin_), imax_(imax_) {}
  template <bool cond = D == 1, std::enable_if_t<cond> * = nullptr>
  constexpr range_t(const irange_t irange)
      : range_t(index_t<D>{irange.imin()}, index_t<D>{irange.imax()}) {
    cxx_assert(irange.istep() == 1);
  }
  constexpr index_t<D> imin() const { return imin_; } // rename to head?
  constexpr index_t<D> imax() const { return imax_; }
  constexpr index_t<D> istep() const { return one(); }
  constexpr index_t<D> shape() const { return adt::max(zero(), imax_ - imin_); }
  constexpr std::size_t size() const { return adt::prod(shape()); }
  constexpr bool empty() const { return adt::any(adt::le(imax_, imin_)); }
  // constexpr index_t<D> operator[](index_t<D> i) const { return imin_ + i; }
  range_t boundary(std::ptrdiff_t f, std::ptrdiff_t d,
                   bool outer = false) const {
    cxx_assert(!empty());
    cxx_assert(f >= 0 && f < 2);
    cxx_assert(d >= 0 && d < std::ptrdiff_t(D));
    range_t bnd(*this);
    if (f == 0) {
      if (outer)
        --bnd.imin_[d];
      bnd.imax_[d] = bnd.imin_[d] + 1;
    } else {
      if (outer)
        ++bnd.imax_[d];
      bnd.imin_[d] = bnd.imax_[d] - 1;
    }
    return bnd;
  }
  constexpr bool operator==(const range_t other) const {
    return (empty() && other.empty()) ||
           (!empty() && !other.empty() && imin_ == other.imin_ &&
            imax_ == other.imax_);
  }
  constexpr bool operator!=(const range_t other) const {
    return !(*this == other);
  }
  friend std::ostream &operator<<(std::ostream &os, const range_t &inds) {
    return os << "range_t(" << inds.imin_ << ":" << inds.imax_ << ")";
  }

private:
  template <std::ptrdiff_t I, typename F, typename... Args>
  std::enable_if_t<(I < 0), void> loop_impl(F &&f, const index_t<D> &ipos,
                                            Args &&... args) const {
    cxx::invoke(std::forward<F>(f), ipos, std::forward<Args>(args)...);
  }
  template <std::ptrdiff_t I, typename F, typename... Args>
  std::enable_if_t<(I >= 0), void> loop_impl(F &&f, index_t<D> ipos,
                                             Args &&... args) const {
    std::ptrdiff_t imin1 = imin()[I];
    std::ptrdiff_t imax1 = imax()[I];
    for (std::ptrdiff_t ipos1 = imin1; ipos1 < imax1; ++ipos1) {
      ipos[I] = ipos1;
      loop_impl<I - 1>(std::forward<F>(f), ipos, std::forward<Args>(args)...);
    }
  }

public:
  template <typename F, typename... Args>
  void loop(F &&f, Args &&... args) const {
    loop_impl<std::ptrdiff_t(D) - 1>(std::forward<F>(f), index_t<D>{},
                                     std::forward<Args>(args)...);
  }

private:
  template <std::ptrdiff_t I, typename F, typename... Args>
  std::enable_if_t<(I < 0), void>
  loop_bnd_impl(F &&f, const index_t<D> &ipos,
                const std::array<std::array<bool, D>, 2> &isbnd,
                Args &&... args) const {
    cxx::invoke(std::forward<F>(f), ipos, isbnd, std::forward<Args>(args)...);
  }
  template <std::ptrdiff_t I, typename F, typename... Args>
  std::enable_if_t<(I >= 0), void>
  loop_bnd_impl(F &&f, index_t<D> ipos,
                std::array<std::array<bool, D>, 2> isbnd,
                Args &&... args) const {
    std::ptrdiff_t imin1 = imin()[I];
    std::ptrdiff_t imax1 = imax()[I];
    if (imin1 >= imax1) {
      // do nothing
    } else if (imin1 == imax1 - 1) {
      ipos[I] = imin1;
      isbnd[0][I] = true;
      isbnd[1][I] = true;
      loop_bnd_impl<I - 1>(std::forward<F>(f), ipos, isbnd,
                           std::forward<Args>(args)...);
    } else {
      ipos[I] = imin1;
      isbnd[0][I] = true;
      isbnd[1][I] = false;
      loop_bnd_impl<I - 1>(std::forward<F>(f), ipos, isbnd,
                           std::forward<Args>(args)...);
      for (std::ptrdiff_t ipos1 = imin1 + 1; ipos1 < imax1 - 1; ++ipos1) {
        ipos[I] = ipos1;
        isbnd[0][I] = false;
        isbnd[1][I] = false;
        loop_bnd_impl<I - 1>(std::forward<F>(f), ipos, isbnd,
                             std::forward<Args>(args)...);
      }
      ipos[I] = imax1 - 1;
      isbnd[0][I] = false;
      isbnd[1][I] = true;
      loop_bnd_impl<I - 1>(std::forward<F>(f), ipos, isbnd,
                           std::forward<Args>(args)...);
    }
  }

public:
  template <typename F, typename... Args>
  void loop_bnd(F &&f, Args &&... args) const {
    loop_bnd_impl<std::ptrdiff_t(D) - 1>(std::forward<F>(f), index_t<D>{},
                                         std::array<std::array<bool, D>, 2>{},
                                         std::forward<Args>(args)...);
  }
};

// Multi-dimensional index space
template <std::size_t D> class space_t {
  range_t<D> allocated_; // array size
  range_t<D> active_;    // active region
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(allocated_, active_);
  }

public:
  constexpr range_t<D> allocated() const { return allocated_; }
  constexpr range_t<D> active() const { return active_; }
  constexpr std::size_t size() const { return active_.size(); }
  constexpr bool empty() const { return active_.empty(); }
  constexpr bool invariant() const noexcept {
    return empty() || (adt::all(ge(active().imin(), allocated().imin()) &&
                                le(active().imax(), allocated().imax())));
  }
  space_t() : space_t(range_t<D>()) {}
  space_t(const range_t<D> &active) : space_t(active, active) {}
  space_t(const range_t<D> &allocated, const range_t<D> &active)
      : allocated_(allocated), active_(active) {
    cxx_assert(invariant());
  }
  constexpr std::ptrdiff_t linear(const index_t<D> &ipos) const {
    cxx_assert(
        adt::all(ge(ipos, active().imin()) && lt(ipos, active().imax())));
    // TODO: optimize this by storing strides instead
    ptrdiff_t ilin = 0;
    for (std::ptrdiff_t d = std::ptrdiff_t(D) - 1; d >= 0; --d) {
      ilin *= allocated().imax()[d] - allocated().imin()[d];
      ilin += ipos[d] - allocated().imin()[d];
    }
    return ilin;
  }
  space_t boundary(std::ptrdiff_t f, std::ptrdiff_t d,
                   bool outer = false) const {
    cxx_assert(!empty());
    cxx_assert(f >= 0 && f < 2);
    cxx_assert(d >= 0 && d < std::ptrdiff_t(D));
    space_t bnd(*this);
    bnd.active_ = bnd.active_.boundary(f, d, outer);
    return bnd;
  }
};

// Multi-dimensional strided range
template <std::size_t D> class steprange_t {
  index_t<D> imin_, imax_, istep_;
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(imin_, imax_, istep_);
  }

public:
  constexpr static index_t<D> zero() { return adt::set<index_t<D>>(0); }
  constexpr static index_t<D> one() { return adt::set<index_t<D>>(1); }
  constexpr bool invariant() const noexcept {
    return adt::all(adt::gt(istep_, 0));
  }
  constexpr steprange_t() : steprange_t(zero()) {}
  constexpr steprange_t(const index_t<D> &imax_) : steprange_t(zero(), imax_) {}
  constexpr steprange_t(const index_t<D> &imin_, const index_t<D> &imax_)
      : steprange_t(imin_, imax_, one()) {}
  constexpr steprange_t(const index_t<D> &imin_, const index_t<D> &imax_,
                        const index_t<D> &istep_)
      : imin_(imin_), imax_(imax_), istep_(istep_) {
    cxx_assert(invariant());
  }
  constexpr index_t<D> imin() const { return imin_; } // rename to head?
  constexpr index_t<D> imax() const { return imax_; }
  constexpr index_t<D> istep() const { return istep_; }
  constexpr index_t<D> shape() const {
    return adt::max(zero(),
                    adt::div_quot(adt::div_ceil(imax_ - imin_, istep_)));
  }
  constexpr std::size_t size() const { return adt::prod(shape()); }
  constexpr bool empty() const { return adt::any(adt::le(imax_, imin_)); }
  // constexpr index_t<D> operator[](index_t<D> i) const {
  //   return imin_ + i * istep_;
  // }
  friend std::ostream &operator<<(std::ostream &os, const steprange_t &inds) {
    return os << "steprange_t(" << inds.imin_ << ":" << inds.imax_ << ":"
              << inds.istep_ << ")";
  }
};
}

#define ADT_INDEX_HPP_DONE
#endif // #ifdef ADT_INDEX_HPP
#ifndef ADT_INDEX_HPP_DONE
#error "Cyclic include dependency"
#endif
