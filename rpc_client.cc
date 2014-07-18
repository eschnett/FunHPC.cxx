#include "rpc_client.hh"

#include "rpc_defs.hh"

#include <atomic>
#include <cstdlib>
#include <random>

namespace rpc {
namespace detail {

#if 0
// Round-robin load distribution
// Note: This puts too much load onto the first processes
int choose_dest() {
  static std::atomic<int> dest(-1);
  int offset = std::rand() % rpc::server->size();
  return rpc::as_transaction(dest, [](int &dest) {
    int result = dest >= 0 ? dest : rpc::server->rank();
    dest = (result + offset) % rpc::server->size();
    return result;
  });
}
#endif

#if 0
// Note: rand() is not thread-safe
int choose_dest() {
  static thread_local int dest(0);
  int offset = std::rand() % rpc::server->size();
  dest = (dest + offset) % rpc::server->size();
  return dest;
}
#endif

int choose_dest() {
  static thread_local std::default_random_engine e((std::random_device()()));
  static thread_local std::unique_ptr<std::uniform_int_distribution<int> > dist;
  if (!dist)
    dist = cxx::make_unique<std::uniform_int_distribution<int> >(
        0, rpc::server->size() - 1);
  return (*dist)(e);
}
}
}
