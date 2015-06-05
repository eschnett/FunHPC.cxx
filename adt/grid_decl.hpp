#ifndef ADT_GRID_DECL_HPP
#define ADT_GRID_DECL_HPP

#include <cstddef>

namespace adt {

template <typename C, typename T, std::size_t D> class grid;
template <typename C, typename T, std::size_t D>
void swap(grid<C, T, D> &x, grid<C, T, D> &y);
}

#define ADT_GRID_DECL_HPP_DONE
#endif // #ifdef ADT_GRID_DECL_HPP
#ifndef ADT_GRID_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
