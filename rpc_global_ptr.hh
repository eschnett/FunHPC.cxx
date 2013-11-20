#ifndef RPC_GLOBAL_PTR_HH
#define RPC_GLOBAL_PTR_HH

#include "rpc_server.hh"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>

#include <cstdint>
#include <iostream>

namespace rpc {
  
  using std::intptr_t;
  
  // A local pointer, valid only in one process, and represented as an
  // opaque bit pattern on all other processes
  template<typename T>
  class local_ptr {
    T* ptr;
  public:
    local_ptr() {}
    local_ptr(T* ptr): ptr(ptr) {}
    T* get() const { return ptr; }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, unsigned int version) const
    {
      // TODO: use serialization::binary_object instead?
      { intptr_t iptr = (intptr_t)ptr; ar << iptr; }
    }
    template<class Archive>
    void load(Archive& ar, unsigned int version)
    {
      { intptr_t iptr; ar >> iptr; ptr = (T*)iptr; }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER();
  };
  
  // A global pointer, represented as a combination of a local pointer
  // and a process rank describing where the pointer is valid, i.e.
  // where the pointee lives.
  template<typename T>
  class global_ptr {
    local_ptr<T> ptr;
    int proc;
  public:
    global_ptr(): proc(-1) {}
    global_ptr(T* ptr): ptr(ptr), proc(server->rank()) {}
    bool is_valid() const { return proc>=0; }
    bool is_local() const { return proc==server->rank(); }
    T* get() const { return is_local() ? ptr.get() : nullptr; }
    int get_proc() const { return proc; }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int version)
    {
      ar & ptr & proc;
    }
  };
  
  template<typename T> global_ptr<T> make_global(T* t) { return t; }
}

#endif  // #ifndef RPC_GLOBAL_PTR_HH
