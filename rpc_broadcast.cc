#include "rpc_broadcast.hh"

#include "rpc_action.hh"

#include <cereal/types/vector.hpp>

#include <numeric>

namespace rpc {

std::vector<int> find_all_threads_partial(int dmin, int dmax);
RPC_DECLARE_ACTION(find_all_threads_partial);

std::vector<int> find_all_threads_partial(int dmin, int dmax) {
  RPC_ASSERT(dmax > dmin);
  if (dmin + 1 == dmax) {
    auto nthreads = thread::hardware_concurrency();
    return std::vector<int>(nthreads, server->rank());
  }
  auto dmed = (dmin + dmax) / 2;
  auto freshi =
      async(remote::async, dmed, find_all_threads_partial_action(), dmed, dmax);
  auto res = find_all_threads_partial(dmin, dmed);
  auto reshi = freshi.get();
  res.insert(res.end(), reshi.begin(), reshi.end());
  return res;
}

std::vector<int> find_all_threads() {
  return find_all_threads_partial(0, server->size());
}

std::vector<int> find_all_processes() {
  std::vector<int> procs(rpc::server->size());
  std::iota(procs.begin(), procs.end(), 0);
  return procs;
}
}

RPC_IMPLEMENT_ACTION(rpc::find_all_threads_partial);

namespace rpc {
namespace detail {
inline void async_broadcast(int b, int e,
                            const std::shared_ptr<callable_base> &evalptr) {
  int sz = e - b;
  if (sz <= 0)
    return;
  if (sz == 1)
    return evalptr->execute();
  int m = b + sz / 2;
  auto f = async(remote::async, m, async_broadcast_action(), m, e, evalptr);
  async_broadcast(b, m, evalptr);
  f.wait();
}
}
}
RPC_IMPLEMENT_ACTION(rpc::detail::async_broadcast);
