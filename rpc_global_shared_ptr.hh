#ifndef RPC_GLOBAL_SHARED_PTR_HH
#define RPC_GLOBAL_SHARED_PTR_HH

#include "rpc_global_shared_ptr_fwd.hh"

#include "rpc_action.hh"
#include "rpc_call.hh"

namespace rpc {
  
  RPC_DECLARE_ACTION(global_owner_add_ref);
  RPC_DECLARE_ACTION(global_owner_remove_ref);
  
  template<typename T>
  struct global_owner_get_ptr_action:
    public action_impl<global_owner_get_ptr_action<T>,
                       wrap<decltype(&global_owner_get_ptr<T>),
                            &global_owner_get_ptr<T>>>
  {
  };
  
  
  
  template<typename T, typename... As>
  struct make_global_shared_action:
    public action_impl<make_global_shared_action<T, As...>,
                       wrap<decltype(&make_global_shared<T, As...>),
                            &make_global_shared<T, As...>>>
  {
  };
  
  
  
  RPC_DECLARE_ACTION(global_keepalive_destruct);
  RPC_DECLARE_ACTION(global_keepalive_add_then_destruct);
  
  
  
  template<typename T>
  shared_future<global_shared_ptr<T>> global_shared_ptr<T>::local() const
  {
    if (is_local()) return make_ready_future(*this);
    const auto& owner = mgr->owner;
    return async([owner]() -> global_shared_ptr<T> {
        return sync(owner.get_proc(), global_owner_get_ptr_action<T>(), owner);
      });
  }
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::save(Archive& ar, unsigned int version) const
  {
    assert(invariant());
    global_ptr<global_owner_base> send_owner;
    global_ptr<shared_ptr<global_manager>> keepalive;
    if (!*this) {
      send_owner = nullptr;
      keepalive = nullptr;
    } else {
      send_owner = mgr->owner;
      keepalive = new shared_ptr<global_manager>(mgr);
    }
    ar << gptr << send_owner << keepalive;
  }
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::load(Archive& ar, unsigned int version)
  {
    assert(invariant());
    global_ptr<global_owner_base> recv_owner;
    global_ptr<shared_ptr<global_manager>> keepalive;
    ar >> gptr >> recv_owner >> keepalive;
    if (!gptr) {
      // Received nullptr: do nothing
      assert(!recv_owner);
      assert(!keepalive);
    } else {
      // Received non-nullptr:
      assert(recv_owner);
      assert(keepalive);
      // Create manager
      // assert(!recv_owner.is_local());
      // TODO: look for an existing local manager
      mgr = make_shared<global_manager>(recv_owner);
      // Register manager with owner, then delete keepalive
      detached(recv_owner.get_proc(),
               global_keepalive_add_then_destruct_action(),
               recv_owner, keepalive);
    }
    assert(invariant());
  }
  
}



#define RPC_GLOBAL_SHARED_HH_DONE
#else
#  ifndef RPC_GLOBAL_SHARED_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_GLOBAL_SHARED_PTR_HH
