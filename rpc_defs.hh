#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include <boost/mpi.hpp>

namespace rpc {
  
  using namespace boost;
  
  typedef int proc_t;
  extern mpi::communicator comm;
  
}

#endif  // #ifndef RPC_DEFS_HH
