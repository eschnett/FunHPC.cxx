#ifndef RPC_HWLOC_HH
#define RPC_HWLOC_HH

namespace rpc {
void set_cpu_bindings();
}

#define RPC_HWLOC_HH_DONE
#else
#ifndef RPC_HWLOC_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_HWLOC_HH
