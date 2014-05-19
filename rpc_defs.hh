#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include "cxx_invoke.hh"

#include "rpc_future.hh"

namespace rpc {
  
  // TODO: Implement "futurize"
  // TODO: move with_lock to cxx_utils.hh
  
  
  
  template<typename M, typename F, typename... As>
  auto with_lock(M& m, const F& f, As&&... args) ->
    typename invoke_of<F, As...>::type
  {
    lock_guard<decltype(m)> g(m);
    return invoke(f, std::forward<As>(args)...);
  }
  
  extern mutex io_mutex;
  
}

#define RPC_DEFS_HH_DONE
#else
#  ifndef RPC_DEFS_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_DEFS_HH
