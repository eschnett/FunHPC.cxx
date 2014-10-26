#ifndef RPC_CALL_HH
#define RPC_CALL_HH

#include "rpc_call_fwd.hh"

#include "rpc_client_fwd.hh"
#include "rpc_defs.hh"
#include "rpc_future_fwd.hh"
#include "rpc_global_ptr_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"
#include "rpc_server.hh"
#include "rpc_thread.hh"

#include "cxx_tuple.hh"
#include "cxx_utils.hh"

#include <cereal/archives/binary.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/tuple.hpp>

#include <cassert>
#include <chrono>
#include <functional>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace rpc {

////////////////////////////////////////////////////////////////////////////////

template <typename F, typename... As>
auto sync(rlaunch policy, int dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value,
                               typename cxx::invoke_of<F, As...>::type>::type {
  RPC_ASSERT(policy == rlaunch::sync);
#ifndef RPC_DISABLE_CALL_SHORTCUT
  if (dest == server->rank()) {
    return F()(std::forward<As>(args)...);
  }
#endif
  typedef typename cxx::invoke_of<F, As...>::type R;
  auto p = new promise<R>;
  auto f = p->get_future();
  server->call(dest, std::make_shared<typename F::evaluate>(
                         p, std::forward<As>(args)...));
  return f.get();
}

template <typename F, typename... As>
auto sync(rlaunch policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value,
                               typename cxx::invoke_of<F, As...>::type>::type {
  return sync(policy, dest.get(), F(), std::forward<As>(args)...);
}

template <typename F, typename... As>
auto detached(rlaunch policy, int dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value, void>::type {
  RPC_ASSERT(policy == rlaunch::detached);
#ifndef RPC_DISABLE_CALL_SHORTCUT
  if (dest == server->rank()) {
    thread(F(), std::forward<As>(args)...).detach();
    return;
  }
#endif
  typedef typename cxx::invoke_of<F, As...>::type R;
  promise<R> *p = nullptr;
  server->call(dest, std::make_shared<typename F::evaluate>(
                         p, std::forward<As>(args)...));
}

template <typename F, typename... As>
auto detached(rlaunch policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value, void>::type {
  RPC_ASSERT(policy == rlaunch::detached);
  if (future_is_ready(dest)) {
    return detached(policy, dest.get(), F(), std::forward<As>(args)...);
  }
  typedef typename cxx::invoke_of<F, As...>::type R;
  promise<R> *p = nullptr;
  auto evalptr =
      std::make_shared<typename F::evaluate>(p, std::forward<As>(args)...);
  thread([dest, evalptr]() {
#ifndef RPC_DISABLE_CALL_SHORTCUT
           if (dest.get() == server->rank()) {
             return evalptr->execute_locally();
           }
#endif
           server->call(dest.get(), evalptr);
         }).detach();
}

namespace detail {
template <typename F, typename... As>
auto make_ready_future(F, As &&... args)
    -> typename std::enable_if<
          !std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  return rpc::make_ready_future(F()(std::forward<As>(args)...));
}
template <typename F, typename... As>
auto make_ready_future(F, As &&... args)
    -> typename std::enable_if<
          std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  return F()(std::forward<As>(args)...), rpc::make_ready_future();
}
}

template <typename F, typename... As>
auto async(rlaunch policy, int dest, F, As &&... args)
    -> typename std::enable_if<
          is_action<F>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  RPC_ASSERT((policy & (rlaunch::async | rlaunch::deferred | rlaunch::sync)) !=
             rlaunch(0));
  RPC_ASSERT((policy & ~(rlaunch::async | rlaunch::deferred | rlaunch::sync)) ==
             rlaunch(0));
  bool is_deferred = policy == rlaunch::deferred;
  bool is_sync = (policy & rlaunch::sync) == rlaunch::sync;
  typedef typename cxx::invoke_of<F, As...>::type R;
#ifndef RPC_DISABLE_CALL_SHORTCUT
  if (dest == server->rank()) {
    if (is_sync) {
      return detail::make_ready_future(F(), std::forward<As>(args)...);
    }
    launch lpolicy = is_deferred ? launch::deferred : launch::async;
    return async(lpolicy, F(), std::forward<As>(args)...);
  }
#endif
  auto p = new promise<R>;
  auto evalptr =
      std::make_shared<typename F::evaluate>(p, std::forward<As>(args)...);
  if (is_deferred) {
    // If the promise_deleter is destructed, it will destruct the
    // promise as well. This will happen only when the thread never
    // runs. If the thread runs, it will disable the promise_deleter,
    // so that the promise continues to live.
    auto promise_deleter = std::make_shared<std::unique_ptr<promise<R> > >(p);
    return async(launch::deferred, [dest, evalptr, promise_deleter]() {
      promise_deleter->release(); // disable deleter
      auto f = evalptr->p->get_future();
      server->call(dest, evalptr);
      return f.get();
    });
  }
  auto f = p->get_future();
  server->call(dest, evalptr);
  if (is_sync)
    f.wait();
  return f;
}

template <typename F, typename... As>
auto async(rlaunch policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<
          is_action<F>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  RPC_ASSERT((policy & (rlaunch::async | rlaunch::deferred | rlaunch::sync)) !=
             rlaunch(0));
  RPC_ASSERT((policy & ~(rlaunch::async | rlaunch::deferred | rlaunch::sync)) ==
             rlaunch(0));
  bool is_deferred = policy == rlaunch::deferred;
  bool is_sync = (policy & rlaunch::sync) == rlaunch::sync;
  if (is_sync || future_is_ready(dest)) {
    return async(policy, dest.get(), F(), std::forward<As>(args)...);
  }
  typedef typename cxx::invoke_of<F, As...>::type R;
  auto p = new promise<R>;
  auto evalptr =
      std::make_shared<typename F::evaluate>(p, std::forward<As>(args)...);
  if (is_deferred) {
    auto promise_deleter = std::make_shared<std::unique_ptr<promise<R> > >(p);
    return async(launch::deferred, [dest, evalptr, promise_deleter]() {
      promise_deleter->release(); // disable deleter
      auto f = evalptr->p->get_future();
#ifndef RPC_DISABLE_CALL_SHORTCUT
      if (dest.get() == server->rank()) {
        evalptr->execute_locally();
        return f.get();
      }
#endif
      server->call(dest.get(), evalptr);
      return f.get();
    });
  }
  auto f = p->get_future();
  thread([dest, evalptr]() {
#ifndef RPC_DISABLE_CALL_SHORTCUT
           if (dest.get() == server->rank()) {
             return evalptr->execute_locally();
           }
#endif
           server->call(dest.get(), evalptr);
         }).detach();
  return f;
}

////////////////////////////////////////////////////////////////////////////////

// Call a member action via a client

template <typename F, typename G, typename... As>
auto sync(rlaunch policy, F, G &&global, As &&... args)
    -> typename std::enable_if<
          (is_action<F>::value && is_global<G>::value),
          typename cxx::invoke_of<F, G, As...>::type>::type {
  return sync(policy, global.get_proc_future(), F(), std::forward<G>(global),
              std::forward<As>(args)...);
}

template <typename F, typename G, typename... As>
auto detached(rlaunch policy, F, G &&global, As &&... args)
    -> typename std::enable_if<(is_action<F>::value && is_global<G>::value),
                               void>::type {
  return detached(policy, global.get_proc_future(), F(),
                  std::forward<G>(global), std::forward<As>(args)...);
}

template <typename F, typename G, typename... As>
auto async(rlaunch policy, F, G &&global, As &&... args)
    -> typename std::enable_if<
          (is_action<F>::value && is_global<G>::value),
          future<typename cxx::invoke_of<F, G, As...>::type> >::type {
  return async(policy, global.get_proc_future(), F(), std::forward<G>(global),
               std::forward<As>(args)...);
}
}

namespace cereal {
namespace detail {

template <typename F, typename R, typename... As>
struct binding_name<rpc::action_evaluate<F, R, As...> > {
  static constexpr const char *name() {
    return typeid(rpc::action_evaluate<F, R, As...>).name();
  }
};

template <typename F, typename R>
struct binding_name<rpc::action_finish<F, R> > {
  static constexpr const char *name() {
    return typeid(rpc::action_finish<F, R>).name();
  }
};
}
}

#define RPC_CALL_HH_DONE
#else
#ifndef RPC_CALL_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CALL_HH
