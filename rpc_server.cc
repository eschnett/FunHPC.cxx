#include "rpc_server.hh"

namespace rpc {
  
  mutex io_mutex;
  
  abstract_server* server = nullptr;
  
}
