#ifndef QTHREAD_THREAD_FWD_HH
#define QTHREAD_THREAD_FWD_HH

#include "qthread_future_fwd.hh"

#include "cxx_invoke.hh"
#include "cxx_utils.hh"



namespace qthread {
  
  enum class launch { async, deferred, sync };
  
  template<typename F, typename... As> 
  auto async(launch policy, const F& func, As&&... args) ->
    future<typename rpc::invoke_of<F, As...>::type>;
  
  template<typename F, typename... As> 
  auto async(const F& func, As&&... args) ->
    typename std::enable_if<!std::is_same<F, launch>::value,
                            future<typename rpc::invoke_of<F, As...>::type>
                            >::type;
  
}

#define QTHREAD_THREAD_FWD_HH_DONE
#else
#  ifndef QTHREAD_THREAD_FWD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_THREAD_FWD_HH
