#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
#include "rpc_global_shared_ptr.hh"

#include <boost/serialization/export.hpp>

#include <utility>

namespace rpc {
  
  template<typename T>
  global_shared_ptr<T>
  client_get_global_shared(const global_ptr<client<T> >& ptr)
  {
    auto res = ptr->get_global_shared();
    delete ptr.get();
    return res;
  }
  
  template<typename T>
  struct client_get_global_shared_action:
    public action_impl<client_get_global_shared_action<T>,
                       wrap<decltype(&client_get_global_shared<T>),
                            &client_get_global_shared<T> > >
  {
  };
  
  
  
  template<typename T>
  template<class Archive>
  void client<T>::save(Archive& ar, unsigned int version) const
  {
#if defined RPC_HPX || defined RPC_QTHREADS
    bool ready = data.ready();
#else
    // less efficient, but C++11
    bool ready = false;
#endif
    ar << ready;
    if (ready) {
      // Send the global shared pointer of this client
      ar << data.get();
    } else {
      // Create a global pointer to this client, and send it
      auto ptr = make_global<client>(*this);
      ar << ptr;
    }
  }
  
  template<typename T>
  template<class Archive>
  void client<T>::load(Archive& ar, unsigned int version)
  {
    RPC_ASSERT(!*this);
    bool ready;
    ar >> ready;
    if (ready) {
      // Receive global shared pointer, and create a client from it
      global_shared_ptr<T> ptr;
      ar >> ptr;
      *this = ptr;
    } else {
      // Create a future waiting for the global shared pointer of the
      // client
      global_ptr<client> ptr;
      ar >> ptr;
      *this = async(ptr.get_proc(), client_get_global_shared_action<T>(), ptr);
    }
  }
  
  
  
  // template<typename T, typename... As>
  // struct make_client_action:
  //   public action_impl<make_client_action<T, As...>,
  //                      wrap<decltype(&make_client<T, As...>),
  //                           &make_client<T, As...> > >
  // {
  // };
  
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
