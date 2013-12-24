#ifndef RPC_FUTURE_HH
#define RPC_FUTURE_HH

#include "rpc_thread.hh"

#define RPC_FUTURE_HH_DONE
#else
#  ifndef RPC_FUTURE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_FUTURE_HH
