#ifndef ADT_MAYBE_HPP
#define ADT_MAYBE_HPP

#include <adt/either.hpp>
#include <cxx/cassert.hpp>

#include <cereal/access.hpp>

#include <tuple>
#include <type_traits>
#include <utility>

namespace adt {

template <typename T> struct maybe {
  either<std::tuple<>, T> elt;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(elt); }

public:
  typedef T element_type;

  maybe() {}
  maybe(const T &elt) : elt(make_right<std::tuple<>, T>(elt)) {}
  maybe(T &&elt) : elt(make_right<std::tuple<>, T>(std::move(elt))) {}
  void swap(maybe &other) {
    using std::swap;
    swap(elt, other.elt);
  }

  void reset() { elt = {}; }
  bool nothing() const noexcept { return elt.left(); }
  bool just() const noexcept { return elt.right(); }

  const T &get_just() const {
    cxx_assert(elt.right());
    return elt.get_right();
  }
  T &get_just() {
    cxx_assert(elt.right());
    return elt.get_right();
  }

  bool operator==(const maybe &other) const { return elt == other.elt; }
  bool operator!=(const maybe &other) const { return !(*this == other); }
  bool operator<(const maybe &other) const { return elt < other.elt; }
  bool operator>(const maybe &other) const { return other < *this; }
  bool operator<=(const maybe &other) const { return !(*this > other); }
  bool operator>=(const maybe &other) const { return !(*this < other); }
};
template <typename T> void swap(maybe<T> &x, maybe<T> &y) { x.swap(y); }

template <typename T> maybe<T> make_nothing() {
  maybe<T> res;
  return res;
}
template <typename T, typename... Args> maybe<T> make_just(Args &&... args) {
  maybe<T> res;
  res.elt = make_right<std::tuple<>, T>(std::forward<Args>(args)...);
  return res;
}
}

#define ADT_MAYBE_HPP_DONE
#endif // #ifdef ADT_MAYBE_HPP
#ifndef ADT_MAYBE_HPP_DONE
#error "Cyclic include dependency"
#endif
