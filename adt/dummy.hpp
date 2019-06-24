#ifndef ADT_DUMMY_HPP
#define ADT_DUMMY_HPP

#include <cereal/access.hpp>

namespace adt {
struct dummy {
  template <typename Archive> void serialize(Archive &ar) { ar(); }
};
inline void swap(dummy &x, dummy &y) {}
} // namespace adt

#define ADT_DUMMY_HPP_DONE
#endif // #ifdef ADT_DUMMY_HPP
#ifndef ADT_DUMMY_HPP_DONE
#error "Cyclic include dependency"
#endif
