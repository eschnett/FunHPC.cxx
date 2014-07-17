#include "rpc_client.hh"

#include "rpc_defs.hh"

#include <atomic>
#include <cstdlib>

namespace rpc {
namespace detail {

#if 0
std::atomic<int> dest(-1);

// Round-robin load distribution
int get_next_dest() {
  int offset = std::rand() % rpc::server->size();
  return rpc::as_transaction(dest, [](int &dest) {
    int result = dest >= 0 ? dest : rpc::server->rank();
    dest = (result + offset) % rpc::server->size();
    return result;
  });
}
#endif

int get_next_dest() {
  static thread_local int dest(0);
  int offset = std::rand() % rpc::server->size();
  dest = (dest + offset) % rpc::server->size();
  return dest;
}
}
}
