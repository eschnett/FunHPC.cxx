#ifndef ADT_EMPTY_HPP
#define ADT_EMPTY_HPP

#include <cereal/access.hpp>

namespace adt {

template <typename T> class empty {
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(); }

public:
  empty() {}
  void swap(empty &other) {}

  bool operator==(const empty &other) const { return true; }
  bool operator!=(const empty &other) const { return !(*this == other); }
  bool operator<(const empty &other) const { return false; }
  bool operator>(const empty &other) const { return other < *this; }
  bool operator<=(const empty &other) const { return !(*this > other); }
  bool operator>=(const empty &other) const { return !(*this < other); }
};
template <typename T> void swap(empty<T> &x, empty<T> &y) { x.swap(y); }
}

#define ADT_EMPTY_HPP_DONE
#endif // #ifdef ADT_EMPTY_HPP
#ifndef ADT_EMPTY_HPP_DONE
#error "Cyclic include dependency"
#endif
