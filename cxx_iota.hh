#ifndef CXX_IOTA_HH
#define CXX_IOTA_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_utils.hh"

#include <cereal/archives/binary.hpp>
#include <cereal/access.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <ostream>
#include <set>
#include <vector>
#include <tuple>
#include <type_traits>

namespace cxx {

// iota: (Int -> (Int,Int,Int) -> a) -> (Int,Int,Int) -> m a

// TODO: introduce iota2 (for combining containers)
// TODO: introduce multi-dimensional iotas

template <typename T> struct range_t {
  T imin, imax, istep;
  bool invariant() const {
    // For now, only positive strides are allowed (this could be relaxed)
    return istep > 0;
  }
  range_t(T imin, T imax, T istep = 1) : imin(imin), imax(imax), istep(istep) {
    assert(invariant());
  }
  range_t(T imax) : range_t(0, imax) {}
  range_t() : range_t(0) {}
  bool empty() const { return imax <= imin; }
  T size() const { return empty() ? T(0) : div_ceil(imax - imin, istep); }
  bool is_subrange_of(const range_t &global) const {
    // The range must be contained in the global range
    if (imin < global.imin)
      return false;
    if (imax > global.imax)
      return false;
    // The lower bound of the range must be aligned with the global range
    if (mod_floor(imin - global.imin, global.istep) != 0)
      return false;
    // The stride of the range must be a multiple of the stride of the
    // global range
    if (mod_floor(istep, global.istep) != 0)
      return false;
    return true;
  }
  std::ostream &output(std::ostream &os) const {
    return os << "range_t{" << imin << ":" << imax << ":" << istep << "}";
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(imin, imax, istep);
  }
};
template <typename T>
std::ostream &operator<<(std::ostream &os, const range_t<T> &r) {
  return r.output(os);
}

struct iota_range_t {
  typedef std::ptrdiff_t element_type;
  range_t<element_type> global, local;
  bool invariant() const { return local.is_subrange_of(global); }
  iota_range_t(const range_t<element_type> &global,
               const range_t<element_type> &local)
      : global(global), local(local) {
    assert(invariant());
  }
  iota_range_t(const range_t<element_type> &global)
      : iota_range_t(global, global) {}
  iota_range_t(const range_t<element_type> &global, element_type imin,
               element_type imax, element_type istep = 1)
      : iota_range_t(global, range_t<element_type>(imin, imax, istep)) {}
  iota_range_t(element_type imin, element_type imax, element_type istep = 1)
      : iota_range_t(range_t<element_type>(imin, imax, istep)) {}
  iota_range_t(element_type imax) : iota_range_t(0, imax) {}
  iota_range_t() : iota_range_t(0) {}
  std::ostream &output(std::ostream &os) const {
    return os << "iota_range_t{global:" << global << ",local:" << local << "}";
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(global, local); }
};
inline std::ostream &operator<<(std::ostream &os, const iota_range_t &r) {
  return r.output(os);
}

// array
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_array<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  ptrdiff_t s = range.local.size();
  C<T> rs;
  assert(rs.size() == s);
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, range.local.imin + i * range.local.istep, as...);
  return rs;
}

// function
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_function<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  ptrdiff_t s = range.local.size();
  assert(s == 1);
  return munit<C>(cxx::invoke(f, range.local.imin, as...));
}

// list
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_list<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  C<T> rs;
  for (std::ptrdiff_t i = range.local.imin; i < range.local.imax;
       i += range.local.istep)
    rs.push_back(cxx::invoke(f, i, as...));
  return rs;
}

// set
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_set<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  C<T> rs;
  for (std::ptrdiff_t i = range.local.imin; i < range.local.imax;
       i += range.local.istep)
    rs.insert(cxx::invoke(f, i, as...));
  return rs;
}

// shared_ptr
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_shared_ptr<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  ptrdiff_t s = range.local.size();
  assert(s == 1);
  return munit<C>(cxx::invoke(f, range.local.imin, as...));
}

// vector
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  ptrdiff_t s = range.local.size();
  C<T> rs(s);
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, range.local.imin + i * range.local.istep, as...);
  return rs;
}
}

#endif // #ifndef CXX_IOTA_HH
