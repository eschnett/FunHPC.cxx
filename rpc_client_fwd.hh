#ifndef RPC_CLIENT_FWD_HH
#define RPC_CLIENT_FWD_HH

#include "rpc_defs.hh"
// TODO
//#include "rpc_shared_global_ptr_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"
#define shared_global_ptr global_shared_ptr
#define make_shared_global make_global_shared

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
    client(): client(shared_global_ptr<T>()) {}
    // explicit client(T* ptr): client(shared_global_ptr<T>(ptr)) {}
    client(const shared_ptr<T>& ptr): client(shared_global_ptr<T>(ptr)) {}
    // explicit client(const global_ptr<T>& ptr): client(shared_global_ptr<T>(ptr)) {}
    client(const shared_global_ptr<T>& ptr): data(make_ready_future(ptr)) {}
    client(future<shared_global_ptr<T>> ptr): data(ptr.share()) {}
    client(const shared_future<shared_global_ptr<T>>& ptr): data(ptr) {}
    client(future<client<T>>& ptr):
      data(std::async([ptr]() -> shared_global_ptr<T>
                      { return ptr.get().data.get(); }))
    {
    }
    client(const shared_future<client<T>>& ptr):
      data(std::async([ptr]() -> shared_global_ptr<T>
                      { return ptr.get().data.get(); }))
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
    
    // TODO
    // shared_global_ptr<T> get_shared_global() const { return data.get(); }
    const shared_ptr<T>& get() const { return data.get().get(); }
    T& operator*() const { return *get(); }
    auto operator->() const -> decltype(get()) { return get(); }
    
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
  
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#  ifndef RPC_CLIENT_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_FWD_HH
