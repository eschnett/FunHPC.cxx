#ifndef ADT_GRID2_DECL_HPP
#define ADT_GRID2_DECL_HPP

#include <cstddef>

namespace adt {

template <typename C, typename T, std::size_t D> class grid2;
template <typename C, typename T, std::size_t D>
void swap(grid2<C, T, D> &x, grid2<C, T, D> &y);
} // namespace adt

#define ADT_GRID2_DECL_HPP_DONE
#endif // #ifdef ADT_GRID2_DECL_HPP
#ifndef ADT_GRID2_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
