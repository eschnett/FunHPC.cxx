#ifndef RPC_SERVER_HH
#define RPC_SERVER_HH

#include <boost/shared_ptr.hpp>

#include <functional>
#include <mutex>

namespace rpc {
  
  using std::lock_guard;
  using std::mutex;
  
  
  
  // TODO: use invoke? make it work similar to async; improve async as
  // well so that it can call member functions.
  template<typename M, typename F, typename... As>
  auto with_lock(M& m, const F& f, const As&... args) -> decltype(f(args...))
  {
    lock_guard<decltype(m)> g(m);
    return f(args...);
  }
  
  extern mutex io_mutex;
  


  struct callable_base;
  
  class abstract_server {
    
  protected:
    int rank_, size_;
    
  public:
    abstract_server(): rank_(-1), size_(-1) {}
    
    int rank() const { return rank_; }
    int size() const { return size_; }
    
    typedef std::function<int(int argc, char** argv)> user_main_t;
    
    virtual ~abstract_server() {}
    
    virtual int event_loop(const user_main_t& user_main) = 0;
    
    virtual void call(int dest, boost::shared_ptr<callable_base> func) = 0;
  };
  
  extern abstract_server* server;
  
}

#endif  // RPC_SERVER_HH
