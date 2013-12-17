#include "rpc_global_shared_ptr.hh"

namespace rpc {
  
  void global_owner_add_ref(global_ptr<global_owner_base> owner)
  {
    ++owner->refcount;
  }
  void global_owner_remove_ref(global_ptr<global_owner_base> owner)
  {
    if (--owner->refcount == 0) delete owner.get();
  }
  
  
  
  global_manager::~global_manager()
  {
    detached(owner.get_proc(), global_owner_remove_ref_action(), owner);
  }
  
  
  
  void global_keepalive_destruct(global_ptr<shared_ptr<global_manager>> keepalive)
  {
    delete keepalive.get();
  }
  void global_keepalive_add_then_destruct(global_ptr<global_owner_base> owner,
                                          global_ptr<shared_ptr<global_manager>> keepalive)
  {
    global_owner_add_ref(owner);
    detached(keepalive.get_proc(),
             global_keepalive_destruct_action(), keepalive);
  }
  
}

RPC_IMPLEMENT_ACTION(rpc::global_owner_add_ref);
RPC_IMPLEMENT_ACTION(rpc::global_owner_remove_ref);

RPC_IMPLEMENT_ACTION(rpc::global_keepalive_destruct);
RPC_IMPLEMENT_ACTION(rpc::global_keepalive_add_then_destruct);
