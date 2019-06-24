#ifndef ADT_EXTRA_HPP
#define ADT_EXTRA_HPP

#include <cereal/access.hpp>

#include <utility>

namespace adt {

template <typename E, typename T> class extra {
  E ex;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(ex); }

public:
  typedef T element_type;

  extra() {}
  extra(const E &ex) : ex(ex) {}
  extra(E &&ex) : ex(std::move(ex)) {}
  extra &operator=(const extra &other) { ex = other.ex; }
  extra &operator=(extra &&other) { ex = std::move(other.ex); }
  void swap(extra &other) {
    using std::swap;
    swap(ex, other.ex);
  }

  const E &get_extra() const { return ex; }
  E &get_extra() { return ex; }
};
template <typename E, typename T> void swap(extra<E, T> &x, extra<E, T> &y) {
  x.swap(y);
}
} // namespace adt

#define ADT_EXTRA_HPP_DONE
#endif // #ifdef ADT_EXTRA_HPP
#ifndef ADT_EXTRA_HPP_DONE
#error "Cyclic include dependency"
#endif
