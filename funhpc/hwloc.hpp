#ifndef FUNHPC_HWLOC_HPP
#define FUNHPC_HWLOC_HPP

namespace funhpc {
void hwloc_set_affinity();
}

#define FUNHPC_HWLOC_HPP_DONE
#endif // #ifdef FUNHPC_HWLOC_HPP
#ifndef FUNHPC_HWLOC_HPP_DONE
#error "Cyclic include dependency"
#endif
