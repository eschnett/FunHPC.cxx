#include "rpc_global_shared_ptr.hh"

namespace rpc {
  
  void global_owner_add_ref(global_ptr<global_owner_base> owner)
  {
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " increasing refcount at " << owner << "\n";
    /* TODO */ std::cout << "   " << typeid(*owner).name() << "\n";
    /* TODO */ std::cout << "   " << owner->get_type() << "\n";
    RPC_ASSERT(owner->invariant());
    RPC_ASSERT(owner->refcount>0 && owner->refcount<1000);
    ++owner->refcount;
  }
  void global_owner_remove_ref(global_ptr<global_owner_base> owner)
  {
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " decreasing refcount at " << owner << ": " << typeid(*owner).name() << " " << owner->get_type() << "\n";
    /* TODO */ std::cout << "   refcount=" << owner->refcount << "\n";
    RPC_ASSERT(owner->refcount>0 && owner->refcount<1000);
    if (--owner->refcount == 0) {
      /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " deleting owner at " << owner << ": " << typeid(*owner).name() << " " << owner->get_type() << "\n";
      delete owner.get();
    }
    /* TODO */ std::cout << "   done decreasing\n";
  }
  
  
  
  global_manager::~global_manager()
  {
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " ~global_manager at " << this << " " << typeid(*this).name() << "\n";
    detached(owner.get_proc(), global_owner_remove_ref_action(), owner);
  }
  
  
  
  void global_keepalive_destruct
  (global_ptr<shared_ptr<global_manager> > keepalive)
  {
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " deleting keepalive at " << keepalive << ": " << typeid(*keepalive).name() << " " << *keepalive << "\n";
    delete keepalive.get();
  }
  void global_keepalive_add_then_destruct
  (global_ptr<global_owner_base> owner,
   global_ptr<shared_ptr<global_manager> > keepalive)
  {
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " global_keepalive_add_then_destruct: owner=" << owner << " keepalive=" << keepalive << ": about to add_ref\n";
    global_owner_add_ref(owner);
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " global_keepalive_add_then_destruct: owner=" << owner << " keepalive=" << keepalive << ": about to detach\n";
    detached(keepalive.get_proc(),
             global_keepalive_destruct_action(), keepalive);
    /* TODO */ std::cout << "[" << rpc::server->rank() << "] pid=" << getpid() << " global_keepalive_add_then_destruct: owner=" << owner << " keepalive=" << keepalive << ": done\n";
  }
  
}

RPC_IMPLEMENT_ACTION(rpc::global_owner_add_ref);
RPC_IMPLEMENT_ACTION(rpc::global_owner_remove_ref);

RPC_IMPLEMENT_ACTION(rpc::global_keepalive_destruct);
RPC_IMPLEMENT_ACTION(rpc::global_keepalive_add_then_destruct);
