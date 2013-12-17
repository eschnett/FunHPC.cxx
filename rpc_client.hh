#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
// TODO
//#include "rpc_shared_global_ptr.hh"
#define make_shared_global_action make_global_shared_action
#include "rpc_global_shared_ptr.hh"

#include <boost/serialization/export.hpp>

namespace rpc {
  
  template<typename T, typename... As>
  struct make_client_action:
    public action_impl<make_client_action<T, As...>,
                       wrap<decltype(make_client<T, As...>),
                            make_client<T, As...>>>
  {
  };
  
  template<typename T, typename... As>
  client<T> make_remote_client(int proc, As... args)
  {
    return async(proc, make_shared_global_action<T, As...>(), args...);
  }
  
}



#define RPC_CLIENT_HH_DONE
#else
#  ifndef RPC_CLIENT_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_HH
