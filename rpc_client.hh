#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
#include "rpc_global_shared_ptr.hh"

#include <boost/serialization/export.hpp>

#include <utility>

namespace rpc {
  
  template<typename T, typename... As>
  struct make_client_action:
    public action_impl<make_client_action<T, As...>,
                       wrap<decltype(&make_client<T, As...>),
                            &make_client<T, As...>>>
  {
  };
  
  template<typename T, typename... As>
  client<T> make_remote_client(int proc, const As&... args)
  {
    return async(proc, make_global_shared_action<T, As...>(), args...);
  }
  
}



#define RPC_CLIENT_HH_DONE
#else
#  ifndef RPC_CLIENT_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_HH
