#ifndef RPC_GLOBAL_PTR_HH
#define RPC_GLOBAL_PTR_HH

#include "rpc_server.hh"

#include <boost/serialization/access.hpp>

#include <cstdint>
#include <iostream>

namespace rpc {
  
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
    
    global_ptr(): proc(-1) {}
    global_ptr(T* ptr): proc(server->rank()), iptr((intptr_t)ptr) {}
    
    bool is_empty() const
    {
      return proc<0;
    }
    int get_proc() const
    {
      assert(!is_empty());
      return proc;
    }
    bool is_local() const
    {
      assert(!is_empty());
      return proc == server->rank();
    }
    
    bool operator==(const global_ptr& other) const
    {
      return proc == other.proc && iptr == other.iptr;
    }
    bool operator!=(const global_ptr& other) const
    {
      return ! (*this == other);
    }
    
    operator bool() const
    {
      assert(!is_empty());
      return (bool)(T*)iptr;
    }
    T* get() const
    {
      assert(!is_empty());
      assert(is_local());
      if (!is_local()) return nullptr;
      return (T*)iptr;
    }
    
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
  global_ptr<T> make_global(const As&... args)
  {
    return new T(args...);
  }
  
}

#endif  // #ifndef RPC_GLOBAL_PTR_HH
