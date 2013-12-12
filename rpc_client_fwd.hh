#ifndef RPC_CLIENT_FWD_HH
#define RPC_CLIENT_FWD_HH

#include "rpc_shared_global_ptr_fwd.hh"

#include <boost/serialization/access.hpp>

#include <cassert>
#include <future>

namespace rpc {
  
  using std::future;
  using std::shared_future;
  
  
  
  template<typename T>
  class client {
    mutable shared_future<shared_global_ptr<T>> data;
  public:
    
    // We require explicit conversions for constructors that take
    // ownership
    client() {}
    explicit client(T* ptr): client(shared_global_ptr<T>(ptr)) {}
    client(const shared_ptr<T>& ptr);
    // client(const global_ptr<T>& ptr): client(shared_global_ptr<T>(ptr)) {}
    client(const shared_global_ptr<T>& ptr): data(make_ready_future(ptr)) {}
    client(future<shared_global_ptr<T>> ptr): data(ptr.share()) {}
    client(const shared_future<shared_global_ptr<T>>& ptr): data(ptr) {}
    
    bool is_empty() const { return data.get().is_empty(); }
    int get_proc() const { return data.get().get_proc(); }
    bool is_local() const { return data.get().is_local(); }
    
    bool operator==(const client& other) const
    {
      return data.get() == other.data.get();
    }
    bool operator!=(const client& other) const
    {
      return ! (*this == other);
    }
    
    // TODO
    shared_global_ptr<T> get_shared_global() const { assert(data.valid()); return data.get(); }
    // global_ptr<T> get_global() const
    // {
    //   return get_shared_global().get_global();
    // }
    // shared_ptr<T> get_shared() const
    // {
    //   return get_shared_global().get_shared();
    // }
    T* get() const { return get_shared_global().get(); }
    operator bool() const { return bool(get()); }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
    
    client local() const { return get_shared_global().local(); }
    
    ostream& output(ostream& os) const
    {
      return os << get_shared_global();
    }
    
  private:
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, unsigned int version) const
    {
      ar << data.get();
    }
    template<class Archive>
    void load(Archive& ar, unsigned int version)
    {
      shared_global_ptr<T> ptr;
      ar >> ptr;
      *this = client(ptr);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER();
  };
  
  template<typename T>
  ostream& operator<<(ostream& os, const client<T>& ptr)
  {
    return ptr.output(os);
  }
  
  template<typename T, typename... As>
  client<T> make_client(const As&... args)
  {
    return make_shared_global<T>(args...);
  }
  
  template<typename T, typename... As>
  client<T> make_remote_client(int proc, As... args);
  
  // TODO: don't copy
  template<typename T>
  client<T>::client(const shared_ptr<T>& ptr): client()
  {
    *this = make_client<T>(*ptr);
  }
  
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#  ifndef RPC_CLIENT_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_FWD_HH
