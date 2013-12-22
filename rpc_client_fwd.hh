#ifndef RPC_CLIENT_FWD_HH
#define RPC_CLIENT_FWD_HH

#include "rpc_defs.hh"
#include "rpc_global_shared_ptr_fwd.hh"

#include "qthread.hh"

#include <boost/serialization/access.hpp>

#include <cassert>

namespace rpc {
  
  using namespace qthread;
  
  using qthread::future;
  using qthread::shared_future;
  
  
  
  template<typename T>
  class client {
    // gcc 4.7 thinks that shared_future::get is non-const
    mutable shared_future<global_shared_ptr<T>> data;
  public:
    
    // We require explicit conversions for constructors that take
    // ownership
    client(): client(global_shared_ptr<T>()) {}
    client(const shared_ptr<T>& ptr): client(global_shared_ptr<T>(ptr)) {}
    client(const global_shared_ptr<T>& ptr): data(make_ready_future(ptr)) {}
    client(future<global_shared_ptr<T>> ptr): data(ptr.share()) {}
    client(const shared_future<global_shared_ptr<T>>& ptr): data(ptr) {}
    client(future<client<T>>& ptr):
      data(qthread::async([ptr]() -> global_shared_ptr<T>
                          { return ptr.get().data.get(); }))
    {
    }
    client(const shared_future<client<T>>& ptr):
      // gcc 4.7 thinks that shared_future::get is non-const
      data(qthread::async([ptr]() -> global_shared_ptr<T>
                          { auto ptr1=ptr; return ptr1.get().data.get(); }))
    {
    }
    
    operator bool() const { return bool(data.get()); }
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
    
    const shared_ptr<T>& get() const { return data.get().get(); }
    T& operator*() const { return *get(); }
    auto operator->() const -> decltype(this->get()) { return get(); }
    
    client local() const { return data.get().local(); }
    
    ostream& output(ostream& os) const
    {
      return os << data.get();
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
      global_shared_ptr<T> ptr;
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
    return make_global_shared<T>(args...);
  }
  
  template<typename T, typename... As>
  client<T> make_remote_client(int proc, As... args);
  
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#  ifndef RPC_CLIENT_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_FWD_HH
