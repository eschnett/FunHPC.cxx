#include "rpc_broadcast.hh"

#include "rpc_action.hh"

#include <boost/serialization/vector.hpp>

namespace rpc {
  
  vector<int> find_all_threads_partial(int dmin, int dmax);
  RPC_DECLARE_ACTION(find_all_threads_partial);
  
  vector<int> find_all_threads_partial(int dmin, int dmax)
  {
    assert(dmax > dmin);
    if (dmin+1 == dmax) {
      auto nthreads = thread::hardware_concurrency();
      return vector<int>(nthreads, server->rank());
    }
    auto dmed = (dmin + dmax) / 2;
    auto freshi = async(dmed, find_all_threads_partial_action(), dmed, dmax);
    auto res = find_all_threads_partial(dmin, dmed);
    res.insert(res.end(), freshi.get().begin(), freshi.get().end());
    return res;
  }
  
  vector<int> find_all_threads()
  {
    return find_all_threads_partial(0, server->size());
  }
  
}

RPC_IMPLEMENT_ACTION(rpc::find_all_threads_partial);
