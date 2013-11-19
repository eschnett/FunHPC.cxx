#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include <boost/mpi.hpp>

namespace rpc {
  
  typedef int proc_t;
  extern boost::mpi::communicator comm;
  
}

#endif  // #ifndef RPC_DEFS_HH
