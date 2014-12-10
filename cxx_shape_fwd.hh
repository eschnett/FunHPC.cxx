#ifndef CXX_SHAPE_FWD_HH
#define CXX_SHAPE_FWD_HH

// #include "cxx_foldable.hh"
// #include "cxx_functor.hh"
// #include "cxx_invoke.hh"
// #include "cxx_kinds.hh"

// #include <algorithm>
// #include <array>
// #include <cassert>
// #include <cstddef>
// #include <limits>
// #include <ostream>
// #include <string>
// #include <tuple>

namespace cxx {

template <typename T, std::ptrdiff_t D> class vect;

template <typename T, std::ptrdiff_t D>
auto operator*(const T &a, const vect<T, D> &i);
template <typename T, std::ptrdiff_t D>
auto max(const vect<T, D> &i, const vect<T, D> &j);
template <typename T, std::ptrdiff_t D>
auto min(const vect<T, D> &i, const vect<T, D> &j);
template <typename T, typename U, std::ptrdiff_t D>
auto ifthen(const vect<T, D> &b, const vect<U, D> &i, const vect<U, D> &j);
template <typename T, std::ptrdiff_t D> auto all_of(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto any_of(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D>
auto dot(const vect<T, D> &i, const vect<T, D> &j);
template <typename T, std::ptrdiff_t D> auto maxdir(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto maxval(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto mindir(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto minval(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto prod(const vect<T, D> &i);
template <typename T, std::ptrdiff_t D> auto sum(const vect<T, D> &i);
template <typename Op, typename T, std::ptrdiff_t D>
auto fold(const Op &op, const T &z, const vect<T, D> &i);
template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D>
auto foldMap(const F &f, const Op &op, const R &z, const vect<T, D> &i);
template <typename F, typename T, std::ptrdiff_t D>
auto fmap(const F &f, const vect<T, D> &i);
template <typename F, typename T, std::ptrdiff_t D>
auto fmap2(const F &f, const vect<T, D> &i, const vect<T, D> &j);
template <typename T, std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const vect<T, D> &i);

template <std::ptrdiff_t D> using index = vect<std::ptrdiff_t, D>;

// Cartesian grid shape

template <std::ptrdiff_t D> class grid_region;
template <std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const grid_region<D> &r);

template <typename T, std::ptrdiff_t D> class boundaries;

template <typename T, std::ptrdiff_t D>
std::ostream &operator<<(std::ostream &os, const boundaries<T, D> &bs);

template <typename F, typename Op, typename R, typename T, std::ptrdiff_t D,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z, const boundaries<T, D> &bs,
             const As &... as);
template <typename F, typename T, std::ptrdiff_t D, typename... As>
auto fmap(const F &f, const boundaries<T, D> &bs, const As &... as);
}

#define CXX_SHAPE_FWD_HH_DONE
#else
#ifndef CXX_SHAPE_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_SHAPE_FWD_HH
