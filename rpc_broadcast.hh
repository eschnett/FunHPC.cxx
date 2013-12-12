#ifndef RPC_BROADCAST_HH
#define RPC_BROADCAST_HH

#include "rpc_call.hh"

#include <vector>
#include <type_traits>

namespace rpc {
  
  using std::enable_if;
  using std::is_same;
  using std::vector;
  
  // Broadcast to all processes
  template<typename F, typename... As>
  auto broadcast(const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value,
                       vector<decltype(func(args...))>>::type
  {
    typedef decltype(func(args...)) R;
    vector<future<R>> fs;
    // TODO: use tree
    for (int dest=0; dest<server->size(); ++dest) {
      fs.push_back(async(dest, func, args...));
    }
    return fs;
  }
  
  template<typename F, typename... As>
  auto broadcast_apply(const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value, void>::type
  {
    // TODO: use tree
    for (int dest=0; dest<server->size(); ++dest) {
      apply(dest, func, args...);
    }
  }
  
  template<typename F, typename... As>
  auto broadcast_barrier(const F& func, As... args, int b=0, int e=-1) ->
    typename enable_if<is_base_of<action_base<F>, F>::value, future<void>>::type
  {
    if (e == -1) e = server->size();
    const auto sz = e - b;
    assert(sz > 0);
    // TODO: execute on different processes
    // TODO: create an action for this?
    if (sz == 1) return async(b, func, args...);
    const auto m = b + sz/2;
    const auto fs0 = broadcast_barrier(func, args..., b, m);
    const auto fs1 = broadcast_barrier(func, args..., m, e);
    return std::async([=](){ fs0.wait(); fs1.wait(); });
  }
  
  
  
  // Reduce from futures
  template<typename R, typename I>
  auto reduce1(const R& op, const I& b, const I& e) -> decltype(op(*b, *b))
  {
    const auto sz = e - b;
    assert(sz != 0);
    // TODO: execute on different processes
    if (sz == 1) return *b;
    const auto m = b + sz/2;
    return op(reduce1(op, b, m), reduce1(op, m, e));
  }
  
  template<typename R, typename Z, typename I>
  auto reduce(const R& op, Z& zero, const I& b, const I& e) ->
    decltype(op(*b, *b))
  // TODO: R must be FV(FV,FV)
  // TODO: Z must be FV()
  // TODO: I must be iterator over FV
  // NOTE: this currently also works with V instead of FV
  // TODO: accept global_ptr<future>?
  {
    if (b == e) return zero();
    return reduce1(op, b, e);
  }
  
}

#define RPC_BROADCAST_HH_DONE
#else
#  ifndef RPC_BROADCAST_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_BROADCAST_HH
