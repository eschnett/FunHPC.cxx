#ifndef RPC_GLOBAL_PTR_HH
#define RPC_GLOBAL_PTR_HH

#include "rpc_defs.hh"

#include <boost/serialization/access.hpp>
#include <boost/serialization/ephemeral.hpp>

#include <iostream>

namespace rpc {
  
  using std::cout;
  
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
      intptr_t iptr = (intptr_t)ptr;
      ar << iptr;
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
    global_ptr(T* ptr): ptr(ptr), proc(comm.rank()) {}
    T* get() const { return comm.rank()==proc ? ptr.get() : nullptr; }
    proc_t get_proc() const { return proc; }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int version) const
    {
      cout << "global_ptr.serialize.0\n";
      ar & ptr & proc;
      cout << "global_ptr.serialize.1\n";
    }
  };
  
}

#endif  // #ifndef RPC_GLOBAL_PTR_HH
