#ifndef RPC_GLOBAL_SHARED_PTR_FWD_HH
#define RPC_GLOBAL_SHARED_PTR_FWD_HH

#include "rpc_global_ptr_fwd.hh"

#include <boost/make_shared.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/shared_ptr.hpp>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <future>
  
namespace rpc {
  
  using boost::make_shared;
  using boost::shared_ptr;
  
  using std::atomic;
  using std::shared_future;
  
  
  
  // A global shared pointer
  
  // Design:
  //
  // On the node where the pointee lives, there is a manager object
  // controlling the pointee. This manager object also contains a
  // reference count for all remote manager objects. The manager
  // object's desctructor also destructs the pointee.
  //
  // On each node, there is at least on manager object. The manager's
  // destructor decreases the owning manager's reference count.
  
  // Names:
  // - T: object type
  // - M: manager
  // - O: owner
  
  // Layout:
  //
  // Pointer:
  //    - shared_ptr<T> ptr, for efficient access if local
  //    - shared_ptr<M> mgr
  //
  // Manager:
  //    - global_ptr<O> owner
  //
  // Owner:
  //    - atomic<ptrdiff_t> refcount
  //    - shared_ptr<T> ptr
  
  
  
  // Owner
  // TODO: Put owner into manager
  
  class global_owner_base;
  void global_owner_add_ref(global_ptr<global_owner_base> owner);
  void global_owner_remove_ref(global_ptr<global_owner_base> owner);
  
  class global_owner_base {
    atomic<ptrdiff_t> refcount;
  public:
    virtual bool invariant() const { return refcount>0; }
    global_owner_base(): refcount(1) { assert(refcount>0); }
    virtual ~global_owner_base() { assert(refcount==0); }
    
    global_owner_base(const global_owner_base&) = delete;
    global_owner_base(global_owner_base&&) = delete;
    global_owner_base& operator=(const global_owner_base&) = delete;
    global_owner_base& operator=(global_owner_base&&) = delete;
    
    template<typename T>
    const shared_ptr<T>& get_ptr() const;
    
    friend void global_owner_add_ref(global_ptr<global_owner_base> owner);
    friend void global_owner_remove_ref(global_ptr<global_owner_base> owner);
  };
  
  template<typename T>
  class global_owner: public global_owner_base {
    // TODO: make ptr private
  public:
    shared_ptr<T> ptr;
    virtual bool invariant() const
    {
      return global_owner_base::invariant() && bool(ptr);
    }
    global_owner(const shared_ptr<T>& ptr): ptr(ptr) { assert(invariant()); }
    virtual ~global_owner() {}
    
    global_owner() = delete;
    global_owner(const global_owner&) = delete;
    global_owner(global_owner&&) = delete;
    global_owner& operator=(const global_owner&) = delete;
    global_owner& operator=(global_owner&&) = delete;
  };
  
  template<typename T>
  const shared_ptr<T>& global_owner_base::get_ptr() const
  {
    return ((const global_owner<T>*)this)->ptr;
  }
  
  template<typename T>
  shared_ptr<T> global_owner_get_ptr(global_ptr<global_owner_base> owner)
  {
    return owner->get_ptr<T>();
  }
  
  
  
  // Manager
  
  class global_manager {
    // TODO: make owner private
  public:
    global_ptr<global_owner_base> owner;
    bool invariant() const { return bool(owner); }
    global_manager(const global_ptr<global_owner_base>& owner): owner(owner)
    {
      assert(invariant());
    }
    ~global_manager();
    
    global_manager() = delete;
    global_manager(const global_manager&) = delete;
    global_manager(global_manager&&) = delete;
    global_manager& operator=(const global_manager&) = delete;
    global_manager& operator=(global_manager&&) = delete;
  };
  
  
  
  void global_keepalive_destruct(global_ptr<shared_ptr<global_manager>> keepalive);
  void global_keepalive_add_then_destruct(global_ptr<global_owner_base> owner,
                                          global_ptr<shared_ptr<global_manager>> keepalive);
  
  
  
  // Pointer
  template<typename T>
  class global_shared_ptr {
    global_ptr<T> gptr;
    // shared_ptr<T> sptr;
    shared_ptr<global_manager> mgr;
  public:
    bool invariant() const
    {
      // nullptr:
      if (!*this) return !gptr && !mgr;
      if (!mgr) return false;
      // !local:
      if (!is_local()) return !mgr->owner.is_local();
      if (!mgr->owner.is_local()) return false;
      if (get_global().get() != get_shared().get()) return false;
      return true;
    }
    
    global_shared_ptr(): gptr(), mgr(nullptr) { assert(invariant()); }
    global_shared_ptr(const shared_ptr<T>& ptr):
      gptr(ptr.get()),
      mgr(ptr ? make_shared<global_manager>(new global_owner<T>(ptr)) : nullptr)
    {
      assert(invariant());
    }
    global_shared_ptr(const global_shared_ptr& other):
      gptr(other.gptr), mgr(other.mgr)
    {
      assert(other.invariant());
      assert(invariant());
    }
    global_shared_ptr(global_shared_ptr&& other):
      global_shared_ptr()
    {
      assert(other.invariant());
      assert(invariant());
      gptr = other.gptr;
      other.gptr = nullptr;
      mgr.swap(other.mgr);
      assert(invariant());
    }
    global_shared_ptr& operator=(const global_shared_ptr& other)
    {
      assert(invariant() && other.invariant());
      if (this != &other) {
        gptr = other.gptr;
        mgr = other.mgr;
      }
      assert(invariant());
      return *this;
    }
    global_shared_ptr& operator=(global_shared_ptr&& other)
    {
      assert(invariant() && other.invariant());
      gptr = nullptr;
      mgr.reset();
      gptr = other.gptr;
      other.gptr = nullptr;
      mgr.swap(other.mgr);
      assert(invariant());
      return *this;
    }
    
    // Can check for nullptr even for remote objects
    operator bool() const { return bool(gptr); }
    int get_proc() const { return gptr.get_proc(); }
    bool is_local() const { return gptr.is_local(); }
    
  private:
    const global_ptr<T>& get_global() const { return gptr; }
    const shared_ptr<T>& get_shared() const
    {
      assert(is_local());
      // return sptr;
      static const shared_ptr<T> null_shared(nullptr);
      if (!*this) return null_shared;
      return mgr->owner->get_ptr<T>();
    }
  public:
    
    bool operator==(const global_shared_ptr& other) const
    {
      return gptr == other.gptr;
    }
    bool operator!=(const global_shared_ptr& other) const
    {
      return ! (*this == other);
    }
    
    // Can only get local objects
    const shared_ptr<T>& get() const { return get_shared(); }
    T& operator*() const { return *get(); }
    auto operator->() const -> decltype(this->get()) { return get(); }
    
    auto local() const -> shared_future<global_shared_ptr>;
    
    ostream& output(ostream& os) const
    {
      return os << gptr;
    }
    
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, unsigned int version) const;
    template<class Archive>
    void load(Archive& ar, unsigned int version);
    BOOST_SERIALIZATION_SPLIT_MEMBER();
  };
  
  template<typename T>
  ostream& operator<<(ostream& os, const global_shared_ptr<T>& ptr)
  {
    return ptr.output(os);
  }
  
  
  
  template<typename T, typename... As>
  global_shared_ptr<T> make_global_shared(As... args)
  {
    return make_shared<T>(args...);
  }
}



#define RPC_GLOBAL_SHARED_FWD_HH_DONE
#else
#  ifndef RPC_GLOBAL_SHARED_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_GLOBAL_SHARED_PTR_FWD_HH
