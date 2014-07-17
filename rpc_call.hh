#ifndef RPC_CALL_HH
#define RPC_CALL_HH

#include "rpc_call_fwd.hh"

#include "rpc_client_fwd.hh"
#include "rpc_defs.hh"
#include "rpc_future.hh"
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
#include <utility>

namespace rpc {

// Base class for all callable RPC objects
struct callable_base {
  virtual ~callable_base() {}
  virtual void execute() = 0;
};

// TODO: combine evaluate and finish classes. instead of a promise
// to fill, add a generic continuation to every action. drop the
// _evaluate suffix.

template <typename F, typename R> struct action_finish : public callable_base {
  global_ptr<promise<R> > p;
  R res;
  action_finish() {} // only for cereal::serialize
  action_finish(const global_ptr<promise<R> > &p, const R &res)
      : p(p), res(res) {}
  virtual void execute() {
    p->set_value(std::move(res));
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, res); }
};
template <typename F> struct action_finish<F, void> : public callable_base {
  global_ptr<promise<void> > p;
  action_finish() {} // only for cereal::serialize
  action_finish(const global_ptr<promise<void> > &p) : p(p) {}
  virtual void execute() {
    p->set_value();
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p); }
};

template <typename F, typename R, typename... As>
struct action_evaluate : public callable_base {
  global_ptr<promise<R> > p;
  std::tuple<typename std::decay<As>::type...> args;
  action_evaluate() {} // only for cereal::serialize
  action_evaluate(const global_ptr<promise<R> > &p, const As &... args)
      : p(p), args(args...) {}
  virtual void execute() {
    R res = cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    server->call(p.get_proc(),
                 std::make_shared<typename F::finish>(p, std::move(res)));
  }
  void execute_locally() {
    R res = cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    p->set_value(std::move(res));
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, args); }
};
template <typename F, typename... As>
struct action_evaluate<F, void, As...> : public callable_base {
  global_ptr<promise<void> > p;
  std::tuple<typename std::decay<As>::type...> args;
  action_evaluate() {} // only for cereal::serialize
  action_evaluate(const global_ptr<promise<void> > &p, const As &... args)
      : p(p), args(args...) {}
  // TODO: Allow moving arguments via &&?
  virtual void execute() {
    cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    server->call(p.get_proc(), std::make_shared<typename F::finish>(p));
  }
  void execute_locally() {
    cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    p->set_value();
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, args); }
};

// Base type for all actions
template <typename F> struct action_base {};

// // A base types for all actions with a particular signature
// template <typename R, typename... As> struct action {
//   R operator()(As... as) const = 0;
// };

template <typename T, T F> struct wrap {
  typedef T type;
  static constexpr T value = F;
};

// Template for an action
template <typename F, typename W, typename R, typename... As>
struct action_impl_t : public action_base<F> {
  // Note: The argument types As are defined by the function that is
  // wrapped to form this action. As thus cannot adapt for perfect
  // forwarding.
  R operator()(const As &... args) const { return W::value(args...); }
  typedef action_evaluate<F, R, As...> evaluate;
  typedef action_finish<F, R> finish;
};

// Get the action implementation (with its template arguments) for a
// function wrapper
template <typename F, typename W, typename R, typename... As>
action_impl_t<F, W, R, As...> get_action_impl_t(R(As...));

// TODO: don't expect a wrapper, expect the function instead
// TODO: determine the function's type automatically
template <typename F, typename W>
using action_impl = decltype(get_action_impl_t<F, W>(W::value));

// TODO: Implement actions as function pointers (values) instead of types --
// this seems to work

// Example action definition for a given function "f":
// struct f_action:
//   public rpc::action_impl<f_action, rpc::wrap<decltype(&f), &f> >
// {
// };
// RPC_CLASS_EXPORT(f_action::evaluate);
// RPC_CLASS_EXPORT(f_action::finish);

// Call an action on a given destination

// Whether a type is an action
template <typename T> using is_action = std::is_base_of<action_base<T>, T>;

template <typename F, typename... As>
auto sync(remote policy, int dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value,
                               typename cxx::invoke_of<F, As...>::type>::type {
  RPC_ASSERT(policy == remote::sync);
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
auto sync(remote policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value,
                               typename cxx::invoke_of<F, As...>::type>::type {
  return sync(policy, dest.get(), F(), std::forward<As>(args)...);
}

template <typename F, typename... As>
auto detached(remote policy, int dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value, void>::type {
  RPC_ASSERT(policy == remote::detached);
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
auto detached(remote policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<is_action<F>::value, void>::type {
  RPC_ASSERT(policy == remote::detached);
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
auto async(remote policy, int dest, F, As &&... args)
    -> typename std::enable_if<
          is_action<F>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  RPC_ASSERT((policy & (remote::async | remote::deferred | remote::sync)) !=
             remote(0));
  RPC_ASSERT((policy & ~(remote::async | remote::deferred | remote::sync)) ==
             remote(0));
  bool is_deferred = policy == remote::deferred;
  bool is_sync = (policy & remote::sync) == remote::sync;
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
auto async(remote policy, const shared_future<int> &dest, F, As &&... args)
    -> typename std::enable_if<
          is_action<F>::value,
          future<typename cxx::invoke_of<F, As...>::type> >::type {
  RPC_ASSERT((policy & (remote::async | remote::deferred | remote::sync)) !=
             remote(0));
  RPC_ASSERT((policy & ~(remote::async | remote::deferred | remote::sync)) ==
             remote(0));
  bool is_deferred = policy == remote::deferred;
  bool is_sync = (policy & remote::sync) == remote::sync;
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

// Template for a member action
template <typename F, typename W, typename R, typename T, typename... As>
struct member_action_impl_t : public action_base<F> {
  R operator()(const client<T> &obj, const As &... args) const {
    return (*obj.*W::value)(args...);
  }
  typedef action_evaluate<F, R, client<T>, As...> evaluate;
  typedef action_finish<F, R> finish;
};

// Get the member action implementation (with its template
// arguments) for a member function wrapper
template <typename F, typename W, typename R, typename T, typename... As>
member_action_impl_t<F, W, R, T, As...>
get_member_action_impl_t(R (T::*)(As...));
template <typename F, typename W, typename R, typename T, typename... As>
member_action_impl_t<F, W, R, T, As...>
get_const_member_action_impl_t(R (T::*)(As...) const);

// TODO: don't expect a wrapper, expect the function instead
// TODO: determine the function's type automatically
template <typename F, typename W>
using member_action_impl = decltype(get_member_action_impl_t<F, W>(W::value));
template <typename F, typename W>
using const_member_action_impl =
    decltype(get_const_member_action_impl_t<F, W>(W::value));

// Call a member action via a client

// Whether a type is a global type, and thus provides get_proc()
template <typename T> struct is_global_helper : std::false_type {};
template <typename T>
struct is_global_helper<global_ptr<T> > : std::true_type {};
template <typename T>
struct is_global_helper<global_shared_ptr<T> > : std::true_type {};
template <typename T> struct is_global_helper<client<T> > : std::true_type {};
template <typename T>
struct is_global : is_global_helper<typename std::remove_cv<
                       typename std::remove_reference<T>::type>::type> {};

template <typename F, typename G, typename... As>
auto sync(remote policy, F, G &&global, As &&... args)
    -> typename std::enable_if<
          (is_action<F>::value &&is_global<G>::value),
          typename cxx::invoke_of<F, G, As...>::type>::type {
  return sync(policy, global.get_proc_future(), F(), std::forward<G>(global),
              std::forward<As>(args)...);
}

template <typename F, typename G, typename... As>
auto detached(remote policy, F, G &&global, As &&... args)
    -> typename std::enable_if<(is_action<F>::value &&is_global<G>::value),
                               void>::type {
  return detached(policy, global.get_proc_future(), F(),
                  std::forward<G>(global), std::forward<As>(args)...);
}

template <typename F, typename G, typename... As>
auto async(remote policy, F, G &&global, As &&... args)
    -> typename std::enable_if<
          (is_action<F>::value &&is_global<G>::value),
          future<typename cxx::invoke_of<F, G, As...>::type> >::type {
  return async(policy, global.get_proc_future(), F(), std::forward<G>(global),
               std::forward<As>(args)...);
}
}

#define RPC_CALL_HH_DONE
#else
#ifndef RPC_CALL_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CALL_HH
