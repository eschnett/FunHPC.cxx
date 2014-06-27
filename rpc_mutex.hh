#ifndef RPC_MUTEX_HH
#define RPC_MUTEX_HH

#include "rpc_thread.hh"

#define RPC_MUTEX_HH_DONE
#else
#ifndef RPC_MUTEX_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_MUTEX_HH
