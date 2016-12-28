#ifndef FUNHPC_HWLOC_HPP
#define FUNHPC_HWLOC_HPP

#include <string>

namespace funhpc {
void hwloc_set_affinity();
std::string hwloc_get_cpu_infos();
}

#define FUNHPC_HWLOC_HPP_DONE
#endif // #ifdef FUNHPC_HWLOC_HPP
#ifndef FUNHPC_HWLOC_HPP_DONE
#error "Cyclic include dependency"
#endif
