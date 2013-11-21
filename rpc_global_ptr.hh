#ifndef RPC_GLOBAL_PTR_HH
#define RPC_GLOBAL_PTR_HH

#include "rpc_server.hh"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>

#include <cstdint>
#include <iostream>

namespace rpc {
  
  using std::intptr_t;
  
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
    bool is_valid() const { return proc>=0; }
    int get_proc() const { assert(is_valid()); return proc; }
    bool is_local() const { assert(is_valid()); return proc==server->rank(); }
    T* get() const
    {
      assert(is_local());
      if (!is_local()) return nullptr;
      return (T*)iptr;
    }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int version)
    {
      ar & proc & iptr;
    }
  };
  
  template<typename T> global_ptr<T> make_global(T* t) { return t; }
}

#endif  // #ifndef RPC_GLOBAL_PTR_HH
