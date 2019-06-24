#ifndef FUNHPC_HWLOC_HPP
#define FUNHPC_HWLOC_HPP

#include <string>

namespace funhpc {
namespace hwloc {
void set_all_cpu_affinities();
std::string get_all_cpu_infos();
} // namespace hwloc
} // namespace funhpc

#define FUNHPC_HWLOC_HPP_DONE
#endif // #ifdef FUNHPC_HWLOC_HPP
#ifndef FUNHPC_HWLOC_HPP_DONE
#error "Cyclic include dependency"
#endif
