#ifndef RPC_GLOBAL_SHARED_PTR_HH
#define RPC_GLOBAL_SHARED_PTR_HH

#include "rpc_global_shared_ptr_fwd.hh"

#include "rpc_action.hh"
#include "rpc_call.hh"

namespace rpc {
  
  namespace global_shared {
    void register_then_unregister(const global_ptr<global_manager_base>& owner,
                                  const global_ptr<global_manager_base>& other,
                                  const global_ptr<global_manager_base>& self);
    void unregister(const global_ptr<global_manager_base>& other);
    RPC_DECLARE_ACTION(register_then_unregister);
    RPC_DECLARE_ACTION(unregister);
  }
  
  
  
  template<typename T, typename... As>
  struct make_global_shared_action:
    public action_impl<make_global_shared_action<T, As...>,
                       wrap<decltype(&make_global_shared<T, As...>),
                            &make_global_shared<T, As...> > >
  {
  };
  
  
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::save(Archive& ar, unsigned int version) const
  {
    RPC_ASSERT(invariant());
    const global_ptr<T> gptr = get_global();
    ar << gptr;
    if (gptr) {
      mgr->incref();
      const global_ptr<global_manager_base> owner = mgr->get_owner();
      const global_ptr<global_manager_base> other = mgr;
      ar << owner << other;
    }
  }
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::load(Archive& ar, unsigned int version)
  {
    RPC_ASSERT(invariant());
    RPC_ASSERT(!mgr);
    global_ptr<T> gptr;
    ar >> gptr;
    if (gptr) {
      global_ptr<global_manager_base> owner, other;
      ar >> owner >> other;
      RPC_ASSERT(!other.is_local());
      if (gptr.is_local()) {
        // The object is local: use the owner as manager
        mgr = (global_manager<T>*)owner.get();
        mgr->incref();
        detached(other.get_proc(), global_shared::unregister_action(), other);
      } else {
        mgr = new global_manager<T>(owner, other, gptr);
      }
    }
    RPC_ASSERT(invariant());
  }
  
}



#define RPC_GLOBAL_SHARED_HH_DONE
#else
#  ifndef RPC_GLOBAL_SHARED_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_GLOBAL_SHARED_PTR_HH
