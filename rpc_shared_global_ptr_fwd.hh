#error "DISABLED"

#ifndef RPC_SHARED_GLOBAL_PTR_FWD_HH
#define RPC_SHARED_GLOBAL_PTR_FWD_HH

#include "rpc_global_ptr_fwd.hh"
#include "rpc_server.hh"

#include <boost/make_shared.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/shared_ptr.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <type_traits>

namespace rpc {
  
  using boost::make_shared;
  using boost::shared_ptr;
  
  using std::atomic;
  using std::function;
  using std::future;
  using std::ostream;
  using std::remove_const;
  
  
  
  // A shared global pointer
  
  class global_manager_t {
    // Number of references. Serialized messages count reference.
    atomic<ptrdiff_t> refs;
    // The manager owning the object, necessarily located on the same
    // process as the object. The owner's owner is null.
    global_ptr<global_manager_t> owner;
    // A function that deletes the object (if local), or does nothing
    // (if remote).
    // TODO: Modify to use C++ deleter semantics
    function<void(void)> deleter;
    
  public:
    
    bool invariant() const
    {
      if (owner.is_empty()) return false;
      if (!owner.is_local() && !owner) return false;
      if (owner.is_local() && owner) return false;
      if (refs <= 0) return false;
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
      if (!manager) return global_ptr<global_manager_t>(nullptr);
      assert(manager->invariant());
      return manager->owner;
    }
    
    static void add_ref(global_manager_t* manager)
    {
      if (manager) {
        assert(manager->invariant());
        ++manager->refs;
      }
    }
    static void remove_ref(global_manager_t* manager)
    {
      if (manager) {
        assert(manager->invariant());
        if (--manager->refs == 0) delete manager;
      }
    }
  };
  
  
  
  template<typename T>
  class shared_global_ptr {
    
    global_ptr<T> ptr;
    global_manager_t* manager;
    
    // TODO: use nullptr for the default delete operator?
    static global_manager_t* make_ref(const global_ptr<T>& ptr)
    {
      if (ptr.is_empty() || !ptr) return nullptr;
      T* raw_ptr = ptr.get();
      return new global_manager_t(nullptr, [=](){ delete raw_ptr; });
    }
    // // TODO: Instead of having an empty deleter, use a null manager
    // // and don't count references
    // static global_manager_t* make_pseudo_ref(const global_ptr<T>& ptr)
    // {
    //   if (ptr.is_empty() || !ptr) return nullptr;
    //   return new global_manager_t(nullptr, []{});
    // }
    
  public:
    
    // We don't create proxies for empty or null pointers
    bool invariant() const {
      if (ptr.is_empty() || !ptr) return !manager;
      return !!manager;
    };
    
    // We require explicit conversions for constructors that take
    // ownership
    shared_global_ptr(): manager(nullptr) { assert(invariant()); }
    explicit shared_global_ptr(T* ptr_):
      ptr(ptr_), manager(make_ref(ptr))
    {
      // take ownership
      if (!ptr.is_empty()) assert(ptr.is_local());
      assert(invariant());
    }
    shared_global_ptr(const shared_ptr<T>& ptr);
    explicit shared_global_ptr(const global_ptr<T>& ptr_):
      ptr(ptr_), manager(make_ref(ptr))
    {
      // take ownership
      if (!ptr.is_empty()) assert(ptr.is_local());
      assert(invariant());
    }
    shared_global_ptr(const shared_global_ptr& other):
      ptr(other.ptr), manager(other.manager)
    {
      assert(other.invariant());
      global_manager_t::add_ref(manager);
      assert(invariant());
    }
    shared_global_ptr(shared_global_ptr&& other):
      ptr(other.ptr), manager(other.manager)
    {
      assert(other.invariant());
      other = shared_global_ptr();
      assert(invariant());
    }
    shared_global_ptr& operator=(const shared_global_ptr& other)
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
    shared_global_ptr& operator=(shared_global_ptr&& other)
    {
      assert(invariant());
      assert(other.invariant());
      assert(this != &other);
      ptr = other.ptr;
      manager = other.manager;
      other.ptr = nullptr;
      other.manager = nullptr;
      assert(invariant());
      return *this;
    }
    ~shared_global_ptr()
    {
      assert(invariant());
      global_manager_t::remove_ref(manager);
    }
    
    bool is_empty() const { return ptr.is_empty(); }
    int get_proc() const { return ptr.get_proc(); }
    bool is_local() const { return ptr.is_local(); }
    
    bool operator==(const shared_global_ptr& other) const
    {
      return ptr == other.ptr;
    }
    bool operator!=(const shared_global_ptr& other) const
    {
      return ! (*this == other);
    }
    
    // global_ptr<T> get_global() const { return ptr; }
    shared_ptr<T> get_shared() const
    {
      // TODO: Store a shared_ptr in shared_global_ptr's manager, so
      // that this call to make_shared is not necessary
      assert(!is_empty());
      if (!*this) return nullptr;
      return make_shared<T>(**this);
    }
    T* get() const { return ptr.get(); }
    operator bool() const { return bool(ptr); }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
    
    future<shared_global_ptr<T>> local() const;
    
    ostream& output(ostream& os) const
    {
      return os << ptr;
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
  ostream& operator<<(ostream& os, const shared_global_ptr<T>& ptr)
  {
    return ptr.output(os);
  }
  
  template<typename T, typename... As>
  shared_global_ptr<T> make_shared_global(As... args)
  {
    return shared_global_ptr<T>(make_global<T>(args...));
  }
  
  template<typename T>
  future<shared_global_ptr<T>> shared_global_ptr<T>::local() const
  {
    // TODO: Store a shared_ptr in shared_global_ptr's manager, so
    // that this call to make_shared is not necessary if the pointer
    // is local
    return std::async([](future<global_ptr<T>> localptr)
                      { return shared_global_ptr<T>(localptr.get()); },
                      ptr.local());
  }
  
  template<typename T>
  shared_global_ptr<T>::shared_global_ptr(const shared_ptr<T>& ptr):
    shared_global_ptr()
  // copy the object
  // TODO: store a shared_ptr internally, omit the copy
  {
    *this = make_shared_global<T>(*ptr);
  }
  
}

#define RPC_SHARED_GLOBAL_PTR_FWD_HH_DONE
#else
#  ifndef RPC_SHARED_GLOBAL_PTR_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_SHARED_GLOBAL_PTR_FWD_HH
