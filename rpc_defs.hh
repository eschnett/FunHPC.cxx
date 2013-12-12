#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include <future>
#include <mutex>

namespace rpc {
  
  using std::future;
  using std::lock_guard;
  using std::mutex;
  using std::promise;
  
  
  
  // TODO: provide "then" for futures
  
  template<typename T>
  future<T> make_ready_future(const T& obj)
  {
    promise<T> p;
    p.set_value(obj);
    return p.get_future();
  }
  
  
  
  // TODO: use invoke? make it work similar to async; improve async as
  // well so that it can call member functions.
  template<typename M, typename F, typename... As>
  auto with_lock(M& m, const F& f, const As&... args) -> decltype(f(args...))
  {
    lock_guard<decltype(m)> g(m);
    return f(args...);
  }
  
  extern mutex io_mutex;
  
}

#define RPC_DEFS_HH_DONE
#else
#  ifndef RPC_DEFS_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_DEFS_HH
