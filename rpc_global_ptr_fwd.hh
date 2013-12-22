#ifndef RPC_GLOBAL_PTR_FWD_HH
#define RPC_GLOBAL_PTR_FWD_HH

#include "rpc_server.hh"

#include "qthread.hh"

#include <boost/serialization/access.hpp>

#include <cstdint>
#include <iostream>

namespace rpc {
  
  using qthread::future;
  
  using std::intptr_t;
  using std::ostream;
  
  // A global pointer, represented as a combination of a local pointer
  // and a process rank describing where the pointer is valid, i.e.
  // where the pointee lives.
  template<typename T>
  class global_ptr {
    
    int proc;
    intptr_t iptr;
    
  public:
    
    bool invariant() const
    {
      return (proc == -1) == !iptr;
    }
    
    global_ptr(T* ptr = nullptr):
      proc(ptr ? server->rank() : -1), iptr((intptr_t)ptr)
    {
      assert(invariant());
    }
    
    operator bool() const { return bool(iptr); }
    int get_proc() const
    {
      return proc;
    }
    bool is_local() const
    {
      // Nullptr is always local
      return !*this || proc == server->rank();
    }
    
    bool operator==(const global_ptr& other) const
    {
      return proc == other.proc && iptr == other.iptr;
    }
    bool operator!=(const global_ptr& other) const
    {
      return ! (*this == other);
    }
    
    T* get() const
    {
      assert(is_local());
      return (T*)iptr;
    }
    T& operator*() const { return *get(); }
    auto operator->() const -> decltype(this->get()) { return get(); }
    
    future<global_ptr<T>> local() const;
    
    ostream& output(ostream& os) const
    {
      return os << proc << ":" << (T*)iptr;
    }
    
  private:
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int version)
    {
      ar & proc & iptr;
    }
  };
  
  template<typename T>
  ostream& operator<<(ostream& os, const global_ptr<T>& ptr)
  {
    return ptr.output(os);
  }
  
  template<typename T, typename... As>
  global_ptr<T> make_global(As... args)
  {
    return new T(args...);
  }
  
}

#define RPC_GLOBAL_PTR_FWD_HH_DONE
#else
#  ifndef RPC_GLOBAL_PTR_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_GLOBAL_PTR_FWD_HH
