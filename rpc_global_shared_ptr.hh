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
  
  class global_proxy_t {
    // Number of references. Serialized messages count reference.
    atomic<ptrdiff_t> refs;
    // The proxy owning the object, necessarily located on the same
    // process as the object. The owner's owner is null.
    global_ptr<global_proxy_t> owner;
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
    
    global_proxy_t() = delete;
    global_proxy_t(const global_proxy_t&) = delete;
    global_proxy_t(global_proxy_t&&) = delete;
    global_proxy_t& operator=(const global_proxy_t&) = delete;
    global_proxy_t& operator=(global_proxy_t&&) = delete;
    
    global_proxy_t(const global_ptr<global_proxy_t>& owner,
                   const function<void(void)>& deleter):
      refs(1), owner(owner), deleter(deleter)
    {
      assert(invariant());
    }
    
    ~global_proxy_t();
    
    static global_ptr<global_proxy_t> get_owner(const global_proxy_t* proxy)
    {
      if (!proxy) return nullptr;
      assert(proxy->refs);
      return proxy->owner;
    }
    
    static void add_ref(global_proxy_t* proxy)
    {
      if (proxy) {
        assert(proxy->refs);
        ++proxy->refs;
      }
    }
    static void remove_ref(global_proxy_t* proxy)
    {
      if (proxy) {
        assert(proxy->refs);
        if (--proxy->refs == 0) delete proxy;
      }
    }
  };
  
  inline void remove_ref(global_ptr<global_proxy_t> proxy)
  {
    global_proxy_t::remove_ref(proxy.get());
  }
  struct remove_ref_action:
    public action_impl<remove_ref_action,
                       wrap<decltype(remove_ref), remove_ref>>
  {
  };
  
  inline void add_remove_ref(global_ptr<global_proxy_t> owner,
                             global_ptr<global_proxy_t> sender_proxy)
  {
    global_proxy_t::add_ref(owner.get());
    apply(sender_proxy.get_proc(), remove_ref_action(), sender_proxy);
  }
  struct add_remove_ref_action:
    public action_impl<add_remove_ref_action,
                       wrap<decltype(add_remove_ref), add_remove_ref>>
  {
  };
  
  global_proxy_t::~global_proxy_t()
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
    global_proxy_t* proxy;
    
    static global_proxy_t* make_ref(const global_ptr<T>& ptr)
    {
      if (ptr.is_empty() || !ptr) return nullptr;
      T* raw_ptr = ptr.get();
      return new global_proxy_t(nullptr, [=]{ delete raw_ptr; });
    }
    
  public:
    
    // We don't create proxies for empty or null pointers
    bool invariant() const {
      if (ptr.is_empty() || !ptr) return !proxy;
      return !!proxy;
    };
    
    global_shared_ptr(): proxy(nullptr) { assert(invariant()); }
    global_shared_ptr(const global_ptr<T>& ptr):
      ptr(ptr), proxy(make_ref(ptr))
    {
      if (!ptr.is_empty()) assert(ptr.is_local());
      assert(invariant());
    }
    global_shared_ptr(const global_shared_ptr& other):
      ptr(other.ptr), proxy(other.proxy)
    {
      assert(other.invariant());
      global_proxy_t::add_ref(proxy);
      assert(invariant());
    }
    global_shared_ptr(global_shared_ptr&& other):
      ptr(other.ptr), proxy(other.proxy)
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
      if (proxy && proxy == other.proxy) {
        assert(ptr == other.ptr);
        return *this;
      }
      // Note: The new reference needs to be added before the old one
      // is removed. Otherwise, the destructor of our old object may
      // be triggered, and this may (indirectly) remove the last
      // reference to our new object before we register it.
      global_proxy_t::add_ref(other.proxy);
      global_proxy_t::remove_ref(proxy);
      ptr = other.ptr;
      proxy = other.proxy;
      return *this;
    }
    global_shared_ptr& operator=(global_shared_ptr&& other)
    {
      assert(invariant());
      assert(other.invariant());
      assert(this != &other);
      ptr = other.ptr;
      proxy = other.proxy;
      other = global_shared_ptr();
      assert(invariant());
      return *this;
    }
    ~global_shared_ptr()
    {
      assert(invariant());
      global_proxy_t::remove_ref(proxy);
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
      global_ptr<global_proxy_t> sender_proxy = proxy;
      global_ptr<global_proxy_t> owner = global_proxy_t::get_owner(proxy);
      global_proxy_t::add_ref(proxy);
      ar << ptr << sender_proxy << owner;
    }
    template<class Archive>
    void load(Archive& ar, unsigned int version)
    {
      assert(!proxy);
      global_ptr<global_proxy_t> sender_proxy, owner;
      ar >> ptr >> sender_proxy >> owner;
      const bool sender_is_owner = sender_proxy && !owner;
      if (sender_is_owner) {
        owner = sender_proxy;
      }
      if (sender_proxy) {
        // TODO: keep one proxy per object per process, using a
        // database to look up the existing proxy (if any)
        if (owner.is_local()) {
          proxy = owner.get();
          global_proxy_t::add_ref(proxy);
          apply(sender_proxy.get_proc(), remove_ref_action(), sender_proxy);
        } else {
          proxy = new global_proxy_t(owner, []{});
        }
      }
      assert(invariant());
      if (proxy && !sender_is_owner) {
        // We need to register ourselves with the owner, and need to
        // de-register the message with the sender (in this order). We
        // don't need to do anything if the sender is the owner.
        apply(owner.get_proc(), add_remove_ref_action(), owner, sender_proxy);
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
