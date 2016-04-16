#ifndef ADT_ARRAY_HPP
#define ADT_ARRAY_HPP

#include <cxx/cassert.hpp>
#include <cxx/cstdlib.hpp>
#include <cxx/invoke.hpp>

#include <cereal/types/array.hpp>

#include <algorithm>
#include <array>
#include <ios>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace adt {
namespace detail {
// See also detail::is_array in <fun/array.hpp>
template <typename> struct is_std_array : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};
}
}

namespace std {

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(op std::declval<T>())>>         \
  constexpr auto operator op(const std::array<T, N> &x) {                      \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = op x[i];                                                          \
    return r;                                                                  \
  }
MAKEOP(+)
MAKEOP(-)
MAKEOP(~)
MAKEOP(!)
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>>     \
  constexpr std::array<R, N> operator op(const std::array<T, N> &x,            \
                                         const std::array<U, N> &y) {          \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, std::size_t N, typename U,                                   \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      std::enable_if_t<!std::is_base_of<std::ios_base, T>::value> * = nullptr> \
  constexpr std::array<R, N> operator op(const T &x,                           \
                                         const std::array<U, N> &y) {          \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>>     \
  constexpr std::array<R, N> operator op(const std::array<T, N> &x,            \
                                         const U &y) {                         \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y;                                                        \
    return r;                                                                  \
  }
MAKEOP(+)
MAKEOP(-)
MAKEOP(*)
MAKEOP(/)
MAKEOP(%)
MAKEOP(&)
MAKEOP(|)
MAKEOP (^)
MAKEOP(<<)
MAKEOP(>>)
MAKEOP(&&)
MAKEOP(||)
// These are already provided by std::array, albeit with a different
// meaning; we thus provide functions eq, ne, lt, le, gt, ge instead
// MAKEOP(== )
// MAKEOP(!= )
// MAKEOP(< )
// MAKEOP(<= )
// MAKEOP(> )
// MAKEOP(>= )
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N, typename U>                             \
  std::array<T, N> &operator op(std::array<T, N> &x,                           \
                                const std::array<U, N> &y) {                   \
    for (std::size_t i = 0; i < N; ++i)                                        \
      x[i] op y[i];                                                            \
    return x;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            std::enable_if_t<1 /*!::adt::detail::is_std_array<U>::value*/> * = \
                nullptr>                                                       \
  std::array<T, N> &operator op(std::array<T, N> &x, const U &y) {             \
    for (std::size_t i = 0; i < N; ++i)                                        \
      x[i] op y;                                                               \
    return x;                                                                  \
  }
MAKEOP(+=)
MAKEOP(-=)
MAKEOP(*=)
MAKEOP(/=)
MAKEOP(%=)
MAKEOP(&=)
MAKEOP(|=)
MAKEOP(^=)
MAKEOP(<<=)
MAKEOP(>>=)
#undef MAKEOP
}

namespace adt {

#define MAKEFUN(f, impl)                                                       \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(impl(std::declval<T>()))>>      \
  constexpr std::array<R, N> f(const std::array<T, N> &x) {                    \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = impl(x[i]);                                                       \
    return r;                                                                  \
  }
MAKEFUN(abs, std::abs)
#undef MAKEFUN

#define MAKEFUN(f, field)                                                      \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(std::declval<T>().field)>>      \
  constexpr std::array<R, N> f(const std::array<T, N> &x) {                    \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i].field;                                                       \
    return r;                                                                  \
  }
MAKEFUN(div_quot, quot)
MAKEFUN(div_rem, rem)
#undef MAKEFUN

#define MAKEFUN(f, impl)                                                       \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(                                \
                impl(std::declval<T>(), std::declval<U>()))>>                  \
  constexpr std::array<R, N> f(const std::array<T, N> &x,                      \
                               const std::array<U, N> &y) {                    \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = impl(x[i], y[i]);                                                 \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, std::size_t N, typename U,                                   \
      std::enable_if_t<!std::is_same<T, std::array<U, N>>::value> * = nullptr, \
      typename R =                                                             \
          std::decay_t<decltype(impl(std::declval<T>(), std::declval<U>()))>>  \
  constexpr std::array<R, N> f(const T &x, const std::array<U, N> &y) {        \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = impl(x, y[i]);                                                    \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, std::size_t N, typename U,                                   \
      std::enable_if_t<!std::is_same<U, std::array<T, N>>::value> * = nullptr, \
      typename R =                                                             \
          std::decay_t<decltype(impl(std::declval<T>(), std::declval<U>()))>>  \
  constexpr std::array<R, N> f(const std::array<T, N> &x, const U &y) {        \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = impl(x[i], y);                                                    \
    return r;                                                                  \
  }
MAKEFUN(max, std::max)
MAKEFUN(min, std::min)
MAKEFUN(div_floor, cxx::div_floor)
MAKEFUN(div_ceil, cxx::div_ceil)
MAKEFUN(div_exact, cxx::div_exact)
#undef MAKEFUN

#define MAKEFUNOP(f, op)                                                       \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>>     \
  constexpr std::array<R, N> f(const std::array<T, N> &x,                      \
                               const std::array<U, N> &y) {                    \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            std::enable_if_t<1 /*!::adt::detail::is_std_array<T>::value*/> * = \
                nullptr>                                                       \
  constexpr std::array<R, N> f(const T &x, const std::array<U, N> &y) {        \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            std::enable_if_t<1 /*!::adt::detail::is_std_array<U>::value*/> * = \
                nullptr>                                                       \
  constexpr std::array<R, N> f(const std::array<T, N> &x, const U &y) {        \
    std::array<R, N> r{};                                                      \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y;                                                        \
    return r;                                                                  \
  }
MAKEFUNOP(eq, ==)
MAKEFUNOP(ne, !=)
MAKEFUNOP(lt, <)
MAKEFUNOP(le, <=)
MAKEFUNOP(gt, >)
MAKEFUNOP(ge, >=)
#undef MAKEFUNOP

template <typename R, std::size_t N> constexpr std::array<R, N> array_zero() {
  std::array<R, N> r{};
  r.fill(R(0));
  return r;
}

template <typename R, std::size_t N> constexpr std::array<R, N> array_one() {
  std::array<R, N> r{};
  r.fill(R(1));
  return r;
}

template <typename R, std::size_t N, typename T>
constexpr std::array<R, N> array_set(T &&x) {
  std::array<R, N> r{};
  r.fill(std::forward<T>(x));
  return r;
}

template <typename R, std::size_t N, std::size_t i>
constexpr std::array<R, N> array_dir() {
  static_assert(i >= 0 && i < N, "");
  std::array<R, N> r{};
  r.fill(R(0));
  std::get<i>(r) = R(1);
  return r;
}

template <typename R, std::size_t N>
constexpr std::array<R, N> array_dir(std::size_t i) {
  cxx_assert(i >= 0 && i < N);
  std::array<R, N> r{};
  r.fill(R(0));
  r[i] = R(1);
  return r;
}

template <typename AR,
          std::enable_if<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR zero() {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(0));
  return r;
}

template <typename AR,
          std::enable_if<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR one() {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(1));
  return r;
}

template <typename AR, typename T,
          std::enable_if<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR set(T &&x) {
  AR r{};
  r.fill(std::forward<T>(x));
  return r;
}

template <typename AR,
          std::enable_if<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR dir(std::size_t i) {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(0));
  r[i] = R(1);
  return r;
}

template <std::size_t i, typename T, std::size_t N>
constexpr std::array<T, N - 1> rmdir(const std::array<T, N> &x) {
  static_assert(i >= 0 && i < N, "");
  std::array<T, N - 1> r{};
  for (std::size_t j = 0; j < N - 1; ++j)
    r[j] = x[j + (j >= i)];
  return r;
}

template <typename T, std::size_t N>
constexpr std::array<T, N - 1> rmdir(const std::array<T, N> &x, std::size_t i) {
  cxx_assert(i >= 0 && i < N);
  std::array<T, N - 1> r{};
  for (std::size_t j = 0; j < N - 1; ++j)
    r[j] = x[j + (j >= i)];
  return r;
}

template <std::size_t i, typename T, std::size_t N, typename U>
constexpr std::array<T, N> update(const std::array<T, N> &x, U &&y) {
  static_assert(i >= 0 && i < N, "");
  std::array<T, N> r(x);
  std::get<i>(r) = std::forward<U>(y);
  return r;
}

template <typename T, std::size_t N, typename U>
constexpr std::array<T, N> update(const std::array<T, N> &x, std::size_t i,
                                  U &&y) {
  cxx_assert(i >= 0 && i < N);
  std::array<T, N> r(x);
  r[i] = std::forward<U>(y);
  return r;
}

namespace detail {
template <typename T> struct numeric_limits {
  static constexpr T min() { return std::numeric_limits<T>::lowest(); }
  static constexpr T max() { return std::numeric_limits<T>::max(); }
};

#define MAKELIM(type)                                                          \
  template <> struct numeric_limits<type> {                                    \
    static constexpr type min() {                                              \
      return -std::numeric_limits<type>::infinity();                           \
    }                                                                          \
    static constexpr type max() {                                              \
      return +std::numeric_limits<type>::infinity();                           \
    }                                                                          \
  };
MAKELIM(float)
MAKELIM(double)
MAKELIM(long double)
#undef MAKELIM
}

#define MAKEREDOP(name, op, z)                                                 \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<T>())>>     \
  constexpr R name(const std::array<T, N> &xs) {                               \
    R r(z);                                                                    \
    for (const auto &x : xs)                                                   \
      r = std::move(r) op x;                                                   \
    return r;                                                                  \
  }
MAKEREDOP(sum, +, R(0))
MAKEREDOP(prod, *, R(1))
MAKEREDOP(all, &&, true)
MAKEREDOP(any, ||, false)
#undef MAKEREDOP

#define MAKEREDFUN(name, f, z)                                                 \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(                                \
                (f)(std::declval<T>(), std::declval<T>()))>>                   \
  constexpr R name(const std::array<T, N> &xs) {                               \
    R r(z);                                                                    \
    for (const auto &x : xs)                                                   \
      r = (f)(std::move(r), x);                                                \
    return r;                                                                  \
  }
MAKEREDFUN(maxval, std::max, detail::numeric_limits<R>::min())
MAKEREDFUN(minval, std::min, detail::numeric_limits<R>::max())
#undef MAKEREDFUN
}

namespace std {
template <typename T, std::size_t N>
std::ostream &operator<<(std::ostream &os, const std::array<T, N> &x) {
  auto oldprec = os.precision(std::numeric_limits<T>::max_digits10);
  os << "[";
  for (std::size_t i = 0; i < N; ++i) {
    if (i > 0)
      os << ",";
    os << x[i];
  }
  os << "]";
  os.precision(oldprec);
  return os;
}

template <typename T, std::size_t N>
std::string to_string(const std::array<T, N> &x) {
  ostringstream os;
  os.precision(std::numeric_limits<T>::max_digits10);
  os << x;
  return std::move(os).str();
}
}

namespace adt {
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

template <std::size_t D> using index_t = std::array<std::ptrdiff_t, D>;

template <std::size_t D> class range_t {
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
  constexpr range_t() : range_t(zero()) {}
  constexpr range_t(const index_t<D> &imax_) : range_t(zero(), imax_) {}
  constexpr range_t(const index_t<D> &imin_, const index_t<D> &imax_)
      : range_t(imin_, imax_, one()) {}
  constexpr range_t(const index_t<D> &imin_, const index_t<D> &imax_,
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
  constexpr index_t<D> operator[](index_t<D> i) const {
    return imin_ + i * istep_;
  }
  friend std::ostream &operator<<(std::ostream &os, const range_t &inds) {
    return os << "range_t(" << inds.imin_ << ":" << inds.imax_ << ":"
              << inds.istep_ << ")";
  }
};
}

#define ADT_ARRAY_HPP_DONE
#endif // #ifdef ADT_ARRAY_HPP
#ifndef ADT_ARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
