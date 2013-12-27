#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include "rpc_future.hh"

namespace rpc {
  
  // Determine the class type of a member function
  template<typename T>
  struct class_type;
  template<typename T, typename R, typename... As>
  struct class_type<R(T::*const)(As...)>
  {
    typedef T type;
  };
  template<typename T, typename R, typename... As>
  struct class_type<R(T::*const)(As...)const>
  {
    typedef T type;
  };
  
  // Determine the return type of a function
  template<typename T>
  struct return_type;
  template<typename R, typename... As>
  struct return_type<R(*const)(As...)>
  {
    typedef R type;
  };
  template<typename T, typename R, typename... As>
  struct return_type<R(T::*const)(As...)>
  {
    typedef R type;
  };
  template<typename T, typename R, typename... As>
  struct return_type<R(T::*const)(As...)const>
  {
    typedef R type;
  };
  
  
  
  // template<typename T, typename FUN>
  // auto future_then(future<T>& ftr, const FUN& fun) -> future<decltype(fun(ftr))>
  // {
  //   return async([=](){ return fun(ftr); });
  // }
  // template<typename T, typename FUN>
  // auto future_then(const shared_future<T>& ftr, const FUN& fun) ->
  //   shared_future<decltype(fun(ftr))>
  // {
  //   return async([=](){ return fun(ftr); });
  // }
  
  // TODO: Implement "futurize"
  
  
  
  // TODO: use invoke? make it work similar to async; improve async as
  // well so that it can call member functions.
  template<typename M, typename F, typename... As>
  auto with_lock(M& m, F&& f, As&&... args) ->
    typename std::result_of<F(As...)>::type
  {
    lock_guard<decltype(m)> g(m);
    return std::forward<F>(f)(std::forward<As>(args)...);
  }
  
  extern mutex io_mutex;
  
}

#define RPC_DEFS_HH_DONE
#else
#  ifndef RPC_DEFS_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_DEFS_HH
