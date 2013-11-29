#ifndef RPC_COMPONENT_HH
#define RPC_COMPONENT_HH

#include "rpc_call.hh"
#include "rpc_global_ptr.hh"
#include "rpc_global_shared_ptr.hh"

#include <boost/shared_ptr.hpp>

#include <future>

namespace rpc {
  
  using boost::shared_ptr;
  
  using std::future;
  
  
  
  template<typename T>
  T local_copy_helper(global_ptr<T> ptr)
  {
    return *ptr.get();
  }
  template<typename T>
  struct local_copy_helper_action:
    public action_impl<local_copy_helper_action<T>,
                       wrap<decltype(local_copy_helper<T>),
                            local_copy_helper<T>>>
  {
  };
  
  template<typename T>
  future<T> local_copy(const global_ptr<T>& ptr)
  {
    return async(ptr.get_proc(), local_copy_helper_action<T>(), ptr);
  }
  
  
  
  template<typename T>
  shared_ptr<T> local_ptr_helper(global_shared_ptr<T> ptr)
  {
    // TODO: Store a shared_ptr in global_shared_ptr's manager, so
    // that this call to make_shared is not necessary
    return make_shared<T>(*ptr.get());
  }
  template<typename T>
  struct local_ptr_helper_action:
    public action_impl<local_ptr_helper_action<T>,
                       wrap<decltype(local_ptr_helper<T>), local_ptr_helper<T>>>
  {
  };
  
  template<typename T>
  future<shared_ptr<T>> local_ptr(const global_shared_ptr<T>& ptr)
  {
    return async(ptr.get_proc(), local_ptr_helper_action<T>(), ptr);
  }
  
}

#endif  // #ifndef RPC_COMPONENT_HH
