#ifndef ADT_IDTYPE_HPP
#define ADT_IDTYPE_HPP

#include <cereal/access.hpp>

#include <tuple>
#include <utility>

namespace adt {

template <typename T> class idtype {
  T elt;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(elt); }

public:
  typedef T element_type;

  idtype() {}
  idtype(const T &elt) : elt(elt) {}
  idtype(T &&elt) : elt(std::move(elt)) {}
  void swap(idtype &other) {
    using std::swap;
    swap(elt, other.elt);
  }

  const T &get() const { return elt; }
  T &get() { return elt; }
  // T &&get() && { return elt; }

  bool operator==(const idtype &other) const { return elt == other.elt; }
  bool operator!=(const idtype &other) const { return !(*this == other); }
  bool operator<(const idtype &other) const { return elt < other.elt; }
  bool operator>(const idtype &other) const { return other < *this; }
  bool operator<=(const idtype &other) const { return !(*this > other); }
  bool operator>=(const idtype &other) const { return !(*this < other); }
};
template <typename T> void swap(idtype<T> &x, idtype<T> &y) { x.swap(y); }

// template <typename T> class idtype : std::tuple<T> {};
// template <typename T> void swap(idtype<T> &x, idtype<T> &y) { x.swap(y); }
}

#define ADT_IDTYPE_HPP_DONE
#endif // #ifdef ADT_IDTYPE_HPP
#ifndef ADT_IDTYPE_HPP_DONE
#error "Cyclic include dependency"
#endif
