#ifndef GLOBAL_PTR_HH
#define GLOBAL_PTR_HH

#include <boost/serialization.hpp>

#include "rpc_defs.hh"

namespace rpc {
  
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
      ar << intptr_t(ptr) << rank;
    }
    void load(Archive& ar, unsigned int version)
    {
      { intptr_t iptr; ar >> iptr; ptr = (T*)iptr; }
      ar >> rank;
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
    T* get() const { return comm.rank()==proc ? ptr : nullptr; }
    proc_t get_proc() const { return proc; }
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int version) const
    {
      ar & ptr & proc;
    }
  };
  
}

#endif  // #ifndef GLOBAL_PTR_HH
