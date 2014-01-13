#ifndef RPC_CLIENT_FWD_HH
#define RPC_CLIENT_FWD_HH

#include "rpc_defs.hh"
#include "rpc_global_shared_ptr_fwd.hh"

#include <boost/serialization/access.hpp>

#include <cassert>
#include <utility>

namespace rpc {
  
  template<typename T>
  class client {
    // gcc 4.7 thinks that shared_future::get is non-const
    mutable shared_future<global_shared_ptr<T> > data;
  public:
    typedef T element_type;
    
    // TODO: define then, unwrap for future<client> etc.?
    
    // We require explicit conversions for constructors that take
    // ownership of pointers
    client(): client(global_shared_ptr<T>()) {}
    client(const shared_ptr<T>& ptr): client(global_shared_ptr<T>(ptr)) {}
    client(const global_shared_ptr<T>& ptr): data(make_shared_future(ptr)) {}
    client(const shared_future<global_shared_ptr<T> >& ptr): data(ptr) {}
    client(future<global_shared_ptr<T> >&& ptr): client(ptr.share()) {}
    
    client(const shared_future<shared_ptr<T> >& ptr):
      client(async([=]() -> global_shared_ptr<T> {
            return ptr.get();
          }))
    {
    }
    client(future<shared_ptr<T> >&& ptr): client(ptr.share()) {}
    
    client(const shared_future<client<T> >& ptr):
      client(async([=]() -> global_shared_ptr<T> {
            return ptr.get().data.get();
          }))
    {
    }
    client(future<client<T> >&& ptr): client(ptr.share()) {}
    
    global_shared_ptr<T> get_global_shared() const { return data.get(); }
    
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
    
    void wait() const { data.wait(); }
    const shared_ptr<T>& get() const { return data.get().get(); }
    T& operator*() const { return *get(); }
    auto operator->() const -> decltype(this->get()) { return get(); }
    
    client make_local() const {
      /*TODO*/std::cout << "["<<rpc::server->rank()<<"] client.make_local.0\n";
      data;
      /*TODO*/std::cout << "["<<rpc::server->rank()<<"] client.make_local.1\n";
      data.get();
      /*TODO*/std::cout << "["<<rpc::server->rank()<<"] client.make_local.2\n";
      /*TODO*/struct atexit { ~atexit() { std::cout << "client.make_local.9\n"; } } x;
      return async([](const shared_future<global_shared_ptr<T> >& data) ->
                   global_shared_ptr<T>
                   {
                     return data.get().make_local().get();
                   }, data);
 }
    
    ostream& output(ostream& os) const
    {
      return os << data.get();
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
  client<T> make_remote_client(int proc, const As&... args);
  
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#  ifndef RPC_CLIENT_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CLIENT_FWD_HH
