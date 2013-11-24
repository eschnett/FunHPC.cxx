#ifndef RPC_GLOBAL_SHARED_PTR_HH
#define RPC_GLOBAL_SHARED_PTR_HH

#include "rpc_call.hh"
#include "rpc_global_ptr.hh"
#include "rpc_server.hh"

#include <boost/serialization/access.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>

namespace rpc {
  
  using std::atomic;
  using std::function;
  using std::intptr_t;
  using std::make_shared;
  
  
  
  // A global shared pointer
  
  class global_manager_t {
    // Number of references. Serialized messages count reference.
    atomic<ptrdiff_t> refs;
    // The manager owning the object, necessarily located on the same
    // process as the object. The owner's owner is null.
    global_ptr<global_manager_t> owner;
    // A function that deletes the object (if local), or does nothing
    // (if remote).
    function<void(void)> deleter;
    
  public:
    
    bool invariant() const
    {
      assert(refs > 0);
      if (owner.is_empty()) return false;
      if (!owner.is_local() && !owner) return false;
      // TODO: Do this once there is only one owner per process
      // if (owner.is_local() && owner) return false;
      return true;
    }
    
    global_manager_t() = delete;
    global_manager_t(const global_manager_t&) = delete;
    global_manager_t(global_manager_t&&) = delete;
    global_manager_t& operator=(const global_manager_t&) = delete;
    global_manager_t& operator=(global_manager_t&&) = delete;
    
    global_manager_t(const global_ptr<global_manager_t>& owner,
                   const function<void(void)>& deleter):
      refs(1), owner(owner), deleter(deleter)
    {
      assert(invariant());
    }
    
    ~global_manager_t();
    
    static
    global_ptr<global_manager_t> get_owner(const global_manager_t* manager)
    {
      if (!manager) return nullptr;
      assert(manager->refs);
      return manager->owner;
    }
    
    static void add_ref(global_manager_t* manager)
    {
      if (manager) {
        assert(manager->refs);
        ++manager->refs;
      }
    }
    static void remove_ref(global_manager_t* manager)
    {
      if (manager) {
        assert(manager->refs);
        if (--manager->refs == 0) delete manager;
      }
    }
  };
  
  inline void remove_ref(global_ptr<global_manager_t> manager)
  {
    global_manager_t::remove_ref(manager.get());
  }
  struct remove_ref_action:
    public action_impl<remove_ref_action,
                       wrap<decltype(remove_ref), remove_ref>>
  {
  };
  
  inline void add_remove_ref(global_ptr<global_manager_t> owner,
                             global_ptr<global_manager_t> sender_manager)
  {
    global_manager_t::add_ref(owner.get());
    apply(sender_manager.get_proc(), remove_ref_action(), sender_manager);
  }
  struct add_remove_ref_action:
    public action_impl<add_remove_ref_action,
                       wrap<decltype(add_remove_ref), add_remove_ref>>
  {
  };
  
  global_manager_t::~global_manager_t()
  {
    assert(refs == 0);
    if (owner) {
      apply(owner.get_proc(), remove_ref_action(), owner);
    }
    deleter();
  }
  
  
  
  template<typename T>
  class global_shared_ptr {
    
    global_ptr<T> ptr;
    global_manager_t* manager;
    
    static global_manager_t* make_ref(const global_ptr<T>& ptr)
    {
      if (ptr.is_empty() || !ptr) return nullptr;
      T* raw_ptr = ptr.get();
      return new global_manager_t(nullptr, [=]{ delete raw_ptr; });
    }
    
  public:
    
    // We don't create proxies for empty or null pointers
    bool invariant() const {
      if (ptr.is_empty() || !ptr) return !manager;
      return !!manager;
    };
    
    global_shared_ptr(): manager(nullptr) { assert(invariant()); }
    global_shared_ptr(const global_ptr<T>& ptr):
      ptr(ptr), manager(make_ref(ptr))
    {
      if (!ptr.is_empty()) assert(ptr.is_local());
      assert(invariant());
    }
    global_shared_ptr(const global_shared_ptr& other):
      ptr(other.ptr), manager(other.manager)
    {
      assert(other.invariant());
      global_manager_t::add_ref(manager);
      assert(invariant());
    }
    global_shared_ptr(global_shared_ptr&& other):
      ptr(other.ptr), manager(other.manager)
    {
      assert(other.invariant());
      other = global_shared_ptr();
      assert(invariant());
    }
    global_shared_ptr& operator=(const global_shared_ptr& other)
    {
      assert(invariant());
      assert(other.invariant());
      if (this == &other) return *this;
      if (manager && manager == other.manager) {
        assert(ptr == other.ptr);
        return *this;
      }
      // Note: The new reference needs to be added before the old one
      // is removed. Otherwise, the destructor of our old object may
      // be triggered, and this may (indirectly) remove the last
      // reference to our new object before we register it.
      global_manager_t::add_ref(other.manager);
      global_manager_t::remove_ref(manager);
      ptr = other.ptr;
      manager = other.manager;
      return *this;
    }
    global_shared_ptr& operator=(global_shared_ptr&& other)
    {
      assert(invariant());
      assert(other.invariant());
      assert(this != &other);
      ptr = other.ptr;
      manager = other.manager;
      other = global_shared_ptr();
      assert(invariant());
      return *this;
    }
    ~global_shared_ptr()
    {
      assert(invariant());
      global_manager_t::remove_ref(manager);
    }
    
    bool is_empty() const { return ptr.is_empty(); }
    int get_proc() const { return ptr.get_proc(); }
    bool is_local() const { return ptr.is_local(); }
    
    bool operator==(const global_shared_ptr& other) const
    {
      return ptr == other.ptr;
    }
    bool operator!=(const global_shared_ptr& other) const
    {
      return ! (*this == other);
    }
    
    operator bool() const { return bool(ptr); }
    T* get() const { return ptr.get(); }
    
    
    
  private:
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, unsigned int version) const
    {
      assert(invariant());
      global_ptr<global_manager_t> sender_manager = manager;
      global_ptr<global_manager_t> owner = global_manager_t::get_owner(manager);
      global_manager_t::add_ref(manager);
      ar << ptr << sender_manager << owner;
    }
    template<class Archive>
    void load(Archive& ar, unsigned int version)
    {
      assert(!manager);
      global_ptr<global_manager_t> sender_manager, owner;
      ar >> ptr >> sender_manager >> owner;
      const bool sender_is_owner = sender_manager && !owner;
      if (sender_is_owner) {
        owner = sender_manager;
      }
      if (sender_manager) {
        // TODO: keep one manager per object per process, using a
        // database to look up the existing manager (if any)
        if (owner.is_local()) {
          manager = owner.get();
          global_manager_t::add_ref(manager);
          apply(sender_manager.get_proc(), remove_ref_action(), sender_manager);
        } else {
          manager = new global_manager_t(owner, []{});
        }
      }
      assert(invariant());
      if (manager && !sender_is_owner) {
        // We need to register ourselves with the owner, and need to
        // de-register the message with the sender (in this order). We
        // don't need to do anything if the sender is the owner.
        apply(owner.get_proc(), add_remove_ref_action(), owner, sender_manager);
      }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER();
  };
  
  template<typename T, typename... As>
  global_shared_ptr<T> make_global_shared(const As&... args)
  {
    return global_shared_ptr<T>(global_ptr<T>(new T(args...)));
  }
  
}

BOOST_CLASS_EXPORT(rpc::remove_ref_action::evaluate);
BOOST_CLASS_EXPORT(rpc::remove_ref_action::finish);
BOOST_CLASS_EXPORT(rpc::add_remove_ref_action::evaluate);
BOOST_CLASS_EXPORT(rpc::add_remove_ref_action::finish);

#endif  // #ifndef RPC_GLOBAL_SHARED_PTR_HH
