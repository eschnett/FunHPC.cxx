#ifndef ADT_ARITH_HPP
#define ADT_ARITH_HPP

#include <cxx/cstdlib.hpp>

#include <array>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <vector>

namespace adt {
namespace detail {
// See also detail::is_array in <fun/array.hpp>
template <typename> struct is_std_array : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};
} // namespace detail

namespace detail {
// See also detail::is_vector in <fun/vector.hpp>
template <typename> struct is_vector : std::false_type {};
template <typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator>> : std::true_type {};
} // namespace detail
} // namespace adt

namespace std {

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(op std::declval<T>())>,         \
            typename CR = std::array<R, N>>                                    \
  constexpr CR operator op(const std::array<T, N> &x) {                        \
    CR r{};                                                                    \
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
                                                   op std::declval<U>())>,     \
            typename CR = std::array<R, N>>                                    \
  constexpr CR operator op(const std::array<T, N> &x,                          \
                           const std::array<U, N> &y) {                        \
    CR r{};                                                                    \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, std::size_t N, typename U,                                   \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      typename CR = std::array<R, N>,                                          \
      std::enable_if_t<!std::is_base_of<std::ios_base, T>::value> * = nullptr> \
  constexpr CR operator op(const T &x, const std::array<U, N> &y) {            \
    CR r{};                                                                    \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            typename CR = std::array<R, N>>                                    \
  constexpr CR operator op(const std::array<T, N> &x, const U &y) {            \
    CR r{};                                                                    \
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
MAKEOP(^)
MAKEOP(<<)
MAKEOP(>>)
MAKEOP(&&)
MAKEOP(||)
// These are already provided by std::array, but with a different
// meaning; we thus provide functions eq, ne, lt, le, gt, ge instead
// MAKEOP(==)
// MAKEOP(!=)
// MAKEOP(<)
// MAKEOP(<=)
// MAKEOP(>)
// MAKEOP(>=)
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N, typename U>                             \
  std::array<T, N> &operator op(std::array<T, N> &x,                           \
                                const std::array<U, N> &y) {                   \
    for (std::size_t i = 0; i < N; ++i)                                        \
      x[i] op y[i];                                                            \
    return x;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U>                             \
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

#define MAKEOP(op)                                                             \
  template <typename T, typename Allocator,                                    \
            typename R = std::decay_t<decltype(op std::declval<T>())>,         \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR operator op(const std::vector<T, Allocator> &x) {                         \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = op x[i];                                                          \
    return r;                                                                  \
  }
MAKEOP(+)
MAKEOP(-)
MAKEOP(~)
MAKEOP(!)
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, typename Allocator, typename U, typename AllocatorU,   \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR operator op(const std::vector<T, Allocator> &x,                           \
                 const std::vector<U, AllocatorU> &y) {                        \
    cxx_assert(y.size() == x.size());                                          \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, typename Allocator, typename U,                              \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      typename CR =                                                            \
          std::vector<R, typename Allocator::template rebind<R>::other>,       \
      std::enable_if_t<!std::is_base_of<std::ios_base, T>::value> * = nullptr> \
  CR operator op(const T &x, const std::vector<U, Allocator> &y) {             \
    CR r(y.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <typename T, typename Allocator, typename U,                        \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR operator op(const std::vector<T, Allocator> &x, const U &y) {             \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
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
MAKEOP(^)
MAKEOP(<<)
MAKEOP(>>)
MAKEOP(&&)
MAKEOP(||)
// These are already provided by std::array, but with a different
// meaning; we thus provide functions eq, ne, lt, le, gt, ge instead
// MAKEOP(==)
// MAKEOP(!=)
// MAKEOP(<)
// MAKEOP(<=)
// MAKEOP(>)
// MAKEOP(>=)
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, typename Allocator, typename U, typename AllocatorU>   \
  std::vector<T, Allocator> &operator op(                                      \
      std::vector<T, Allocator> &x, const std::vector<U, AllocatorU> &y) {     \
    cxx_assert(y.size() == x.size());                                          \
    for (std::size_t i = 0; i < x.size(); ++i)                                 \
      x[i] op y[i];                                                            \
    return x;                                                                  \
  }                                                                            \
  template <typename T, typename Allocator, typename U>                        \
  std::vector<T, Allocator> &operator op(std::vector<T, Allocator> &x,         \
                                         const U &y) {                         \
    for (std::size_t i = 0; i < x.size(); ++i)                                 \
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
} // namespace std

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
      typename R =                                                             \
          std::decay_t<decltype(impl(std::declval<T>(), std::declval<U>()))>,  \
      std::enable_if_t<!std::is_same<T, std::array<U, N>>::value> * = nullptr> \
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

#define MAKEFUN(f, impl)                                                       \
  template <typename T, typename Allocator,                                    \
            typename R = std::decay_t<decltype(impl(std::declval<T>()))>,      \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR f(const std::vector<T, Allocator> &x) {                                   \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = impl(x[i]);                                                       \
    return r;                                                                  \
  }
MAKEFUN(abs, std::abs)
#undef MAKEFUN

#define MAKEFUN(f, field)                                                      \
  template <typename T, typename Allocator,                                    \
            typename R = std::decay_t<decltype(std::declval<T>().field)>,      \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR f(const std::vector<T, Allocator> &x) {                                   \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = x[i].field;                                                       \
    return r;                                                                  \
  }
MAKEFUN(div_quot, quot)
MAKEFUN(div_rem, rem)
#undef MAKEFUN

#define MAKEFUN(f, impl)                                                       \
  template <typename T, typename Allocator, typename U, typename AllocatorU,   \
            typename R = std::decay_t<decltype(                                \
                impl(std::declval<T>(), std::declval<U>()))>,                  \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR f(const std::vector<T, Allocator> &x,                                     \
       const std::vector<U, AllocatorU> &y) {                                  \
    cxx_assert(y.size() == x.size());                                          \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = impl(x[i], y[i]);                                                 \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, typename Allocator, typename U,                              \
      std::enable_if_t<!std::is_same<T, std::vector<U, Allocator>>::value> * = \
          nullptr,                                                             \
      typename R =                                                             \
          std::decay_t<decltype(impl(std::declval<T>(), std::declval<U>()))>,  \
      typename CR =                                                            \
          std::vector<R, typename Allocator::template rebind<R>::other>>       \
  CR f(const T &x, const std::vector<U, Allocator> &y) {                       \
    CR r(y.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = impl(x, y[i]);                                                    \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, typename Allocator, typename U,                              \
      std::enable_if_t<!std::is_same<U, std::vector<T, Allocator>>::value> * = \
          nullptr,                                                             \
      typename R =                                                             \
          std::decay_t<decltype(impl(std::declval<T>(), std::declval<U>()))>,  \
      typename CR =                                                            \
          std::vector<R, typename Allocator::template rebind<R>::other>>       \
  CR f(const std::vector<T, Allocator> &x, const U &y) {                       \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
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
  template <typename T, typename Allocator, typename U, typename AllocatorU,   \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>,     \
            typename CR =                                                      \
                std::vector<R, typename Allocator::template rebind<R>::other>> \
  CR f(const std::vector<T, Allocator> &x,                                     \
       const std::vector<U, AllocatorU> &y) {                                  \
    cxx_assert(y.size() == x.size());                                          \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, typename Allocator, typename U,                              \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      typename CR =                                                            \
          std::vector<R, typename Allocator::template rebind<R>::other>,       \
      std::enable_if_t<1 /*!::adt::detail::is_std_vector<T>::value*/> * =      \
          nullptr>                                                             \
  CR f(const T &x, const std::vector<U, Allocator> &y) {                       \
    CR r(y.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, typename Allocator, typename U,                              \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      typename CR =                                                            \
          std::vector<R, typename Allocator::template rebind<R>::other>,       \
      std::enable_if_t<1 /*!::adt::detail::is_std_vector<U>::value*/> * =      \
          nullptr>                                                             \
  CR f(const std::vector<T, Allocator> &x, const U &y) {                       \
    CR r(x.size());                                                            \
    for (std::size_t i = 0; i < r.size(); ++i)                                 \
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

template <typename R, std::size_t N>
constexpr std::array<R, N> array_offset(std::size_t s, std::size_t i) {
  cxx_assert(s >= 0 && s < 2);
  cxx_assert(i >= 0 && i < N);
  std::array<R, N> r{};
  r.fill(R(0));
  r[i] = R(s ? -1 : +1);
  return r;
}

template <typename AR,
          std::enable_if_t<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR zero() {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(0));
  return r;
}

template <typename AR,
          std::enable_if_t<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR one() {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(1));
  return r;
}

template <typename AR, typename T,
          std::enable_if_t<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR set(T &&x) {
  AR r{};
  r.fill(std::forward<T>(x));
  return r;
}

template <typename AR,
          std::enable_if_t<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR dir(std::size_t i) {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(0));
  r[i] = R(1);
  return r;
}

template <typename AR,
          std::enable_if_t<detail::is_std_array<AR>::value> * = nullptr>
constexpr AR offset(std::size_t s, std::size_t i) {
  AR r{};
  typedef typename AR::value_type R;
  r.fill(R(0));
  r[i] = R(s ? -1 : +1);
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
} // namespace detail

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
} // namespace adt

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
} // namespace std

#define ADT_ARITH_HPP_DONE
#endif // #ifdef ADT_ARITH_HPP
#ifndef ADT_ARITH_HPP_DONE
#error "Cyclic include dependency"
#endif
