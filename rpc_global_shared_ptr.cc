#include "rpc_global_shared_ptr.hh"

namespace rpc {
  
  global_manager_base::global_manager_base
  (const global_ptr<global_manager_base>& owner,
   const global_ptr<global_manager_base>& other):
    refcount(1), owner(owner)
  {
    RPC_ASSERT(owner);
    RPC_ASSERT(other);
    // We don't want multiple managers per process, so we forbid this.
    // This is for efficiency only.
    RPC_ASSERT(!owner.is_local());
    RPC_ASSERT(!other.is_local());
    // We assume that other has its refcount artificially increased by
    // one, ensuring that the object won't be destructed before we
    // have registered. We also start out with a reference count that
    // has been artificially increased by one to ensure that we don't
    // de-register with the owner before we actually have registered.
    
    // We register in three steps:
    // 1.  Send message to owner, increasing the refcount there
    // 2a. Send message to other, decreasing the refcount there
    // 2b. Send message (back) to this, decreasing the refcount
    //     there
    if (other != owner) {
      refcount = 2;
      detached(remote::detached, owner.get_proc(),
               global_shared::register_then_unregister_action(),
               owner, other, this);
    }
    RPC_ASSERT(invariant() && refcount>0);
  }
  
  global_manager_base::~global_manager_base()
  {
    RPC_ASSERT(invariant());
    RPC_ASSERT(refcount==0);
    if (!owner.is_local()) {
      detached(remote::detached,
               owner.get_proc(), global_shared::unregister_action(), owner);
    } else {
      RPC_ASSERT(owner.get() == this);
    }
  }
  
  
  
  namespace global_shared {
    void register_then_unregister(const global_ptr<global_manager_base>& owner,
                                  const global_ptr<global_manager_base>& other,
                                  const global_ptr<global_manager_base>& self)
    {
      owner->incref();
      detached(remote::detached, other.get_proc(), unregister_action(), other);
      detached(remote::detached, self.get_proc(), unregister_action(), self);
    }
    
    void unregister(const global_ptr<global_manager_base>& other)
    {
      other->decref();
    }
  }
  
}

RPC_IMPLEMENT_ACTION(rpc::global_shared::register_then_unregister);
RPC_IMPLEMENT_ACTION(rpc::global_shared::unregister);
