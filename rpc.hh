#ifndef RPC_HH
#define RPC_HH

#include "rpc_action.hh"
#include "rpc_broadcast.hh"
#include "rpc_call.hh"
#include "rpc_client.hh"
#include "rpc_defs.hh"
#include "rpc_global_ptr.hh"
#include "rpc_global_shared_ptr.hh"
#include "rpc_server.hh"
#include "rpc_tuple.hh"

#define RPC_HH_DONE
#else
#  ifndef RPC_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_HH
