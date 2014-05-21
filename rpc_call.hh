#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_thread.hh"
#include "rpc_client_fwd.hh"
#include "rpc_defs.hh"
#include "rpc_global_ptr_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"
#include "rpc_server.hh"

#include "cxx_tuple.hh"
#include "cxx_utils.hh"

#include <boost/make_shared.hpp>
// Note: <boost/mpi/packed_[io]archive.hpp> need to be included before
// using the macro BOOST_CLASS_EXPORT
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <cassert>
#include <chrono>
#include <functional>
#include <sstream>
#include <type_traits>
#include <utility>

namespace rpc {
  
  // TODO: don't use using
  using boost::make_shared;
  using boost::shared_ptr;
  
  using std::enable_if;
  using std::is_base_of;
  using std::istringstream;
  using std::ostringstream;
  using std::string;
  using std::tuple;
  
  
  
  // Base class for all callable RPC objects
  struct callable_base {
    virtual ~callable_base() {}
    virtual void execute() = 0;
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version) {}
  };
  
  
  
  template<typename F, typename R>
  struct action_finish: public callable_base {
    global_ptr<promise<R> > p;
    typename std::decay<R>::type res;
    action_finish() {}          // only for boost::serialize
    action_finish(const global_ptr<promise<R> >& p, R res): p(p), res(res) {}
    void execute()
    {
      p->set_value(res);
      delete p.get();
      p = nullptr;
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & p & res;
    }
  };
  template<typename F>
  struct action_finish<F, void>: public callable_base {
    global_ptr<promise<void> > p;
    action_finish() {}          // only for boost::serialize
    action_finish(const global_ptr<promise<void> >& p): p(p) {}
    void execute()
    {
      p->set_value();
      delete p.get();
      p = nullptr;
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & p;
    }
  };
  
  template<typename F, typename R, typename... As>
  struct action_evaluate: public callable_base {
    global_ptr<promise<R> > p;
    tuple<typename std::decay<As>::type...> args;
    action_evaluate() {}        // only for boost::serialize
    action_evaluate(const global_ptr<promise<R> >& p, const As&... args):
      p(p), args(args...) {}
    void execute()
    {
      auto cont = tuple_apply(F(), args);
      if (!p) return;
      server->call(p.get_proc(), make_shared<typename F::finish>(p, cont));
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & p & args;
    }
  };
  template<typename F, typename... As>
  struct action_evaluate<F, void, As...>: public callable_base {
    global_ptr<promise<void> > p;
    tuple<typename std::decay<As>::type...> args;
    action_evaluate() {}        // only for boost::serialize
    action_evaluate(const global_ptr<promise<void> >& p, const As&... args):
      p(p), args(args...) {}
    // TODO: Allow moving arguments via &&?
    void execute()
    {
      tuple_apply(F(), args);
      if (!p) return;
      server->call(p.get_proc(), make_shared<typename F::finish>(p));
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & p & args;
    }
  };
  
  
  
  // Base type for all actions
  template<typename F>
  struct action_base {
  };
  
  template<typename T, T F>
  struct wrap {
    typedef T type;
    // static constexpr T value = F;
    static T get_value() { return F; }
  };
  
  // Template for an action
  template<typename F, typename W, typename R, typename... As>
  struct action_impl_t: public action_base<F> {
    // Note: The argument types As are defined by the function that is
    // wrapped to form this action. As thus cannot adapt for perfect
    // forwarding.
    R operator()(const As&... args) const
    {
      return W::get_value()(args...);
    }
    typedef action_evaluate<F, R, As...> evaluate;
    typedef action_finish<F, R> finish;
  };
  
  // Get the action implementation (with its template arguments) for a
  // function wrapper
  template<typename F, typename W, typename R, typename... As>
  action_impl_t<F, W, R, As...> get_action_impl_t(R(As...));
  
  // TODO: don't expect a wrapper, expect the function instead
  // TODO: determine the function's type automatically
  template<typename F, typename W>
  using action_impl = decltype(get_action_impl_t<F, W>(W::get_value()));
  
  // Example action definition for a given function "f":
  // struct f_action:
  //   public rpc::action_impl<f_action, rpc::wrap<decltype(&f), &f> >
  // {
  // };
  // BOOST_CLASS_EXPORT(f_action::evaluate);
  // BOOST_CLASS_EXPORT(f_action::finish);
  
  
  
  // Remote call policies
  
  enum class remote: int { async = 1, deferred = 2, sync = 4, detached = 8 };
  
  inline constexpr remote operator~(remote a)
  {
    return static_cast<remote>(~ static_cast<int>(a));
  }
  
  inline constexpr remote operator&(remote a, remote b)
  {
    return static_cast<remote>(static_cast<int>(a) & static_cast<int>(b));
  }
  inline constexpr remote operator|(remote a, remote b)
  {
    return static_cast<remote>(static_cast<int>(a) | static_cast<int>(b));
  }
  inline constexpr remote operator^(remote a, remote b)
  {
    return static_cast<remote>(static_cast<int>(a) ^ static_cast<int>(b));
  }
  
  inline remote& operator&=(remote& a, remote b)
  {
    return a = a & b;
  }
  inline remote& operator|=(remote& a, remote b)
  {
    return a = a | b;
  }
  inline remote& operator^=(remote& a, remote b)
  {
    return a = a ^ b;
  }
  
  
  
  // Call an action on a given destination
  
  // Whether a type is a (non-member) action
  template<typename T>
  using is_action = is_base_of<action_base<T>, T>;
  
  template<typename F, typename... As>
  auto sync(remote policy, int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value,
                       typename invoke_of<F, As...>::type>::type
  {
    RPC_ASSERT(policy == remote::sync);
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return F()(std::forward<As>(args)...);
    }
#endif
    typedef typename invoke_of<F, As...>::type R;
    auto p = new promise<R>;
    auto f = p->get_future();
    server->call
      (dest, make_shared<typename F::evaluate>(p, std::forward<As>(args)...));
    return f.get();
  }
  
  template<typename F, typename... As>
  auto detached(remote policy, int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value, void>::type
  {
    RPC_ASSERT(policy == remote::detached);
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      thread(F(), std::forward<As>(args)...).detach();
      return;
    }
#endif
    typedef typename invoke_of<F, As...>::type R;
    promise<R>* p = nullptr;
    server->call
      (dest, make_shared<typename F::evaluate>(p, std::forward<As>(args)...));
  }
  
  template<typename F, typename... As>
  auto async(remote policy, int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value,
                       future<typename invoke_of<F, As...>::type> >::type
  {
    RPC_ASSERT((policy & (remote::async | remote::deferred)) != remote(0));
    RPC_ASSERT((policy & ~(remote::async | remote::deferred)) == remote(0));
    bool is_deferred = (policy & remote::deferred) == remote::deferred;
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      launch lpolicy = is_deferred ? launch::deferred : launch::async;
      return async(lpolicy, F(), std::forward<As>(args)...);
    }
#endif
    typedef typename invoke_of<F, As...>::type R;
    auto p = new promise<R>;
    auto evalptr =
      make_shared<typename F::evaluate>(p, std::forward<As>(args)...);
    if (is_deferred) {
      // TODO: This will leak p if the thread never executes. Need to
      // change F::evaluate to accept global_ptr<shared_ptr<promise<R>
      // > > instead.
      return async(launch::deferred,
                   [dest, evalptr]() {
                     auto f = evalptr->p->get_future();
                     server->call(dest, evalptr);
                     return f.get();
                   });
    } else {
      auto f = p->get_future();
      server->call(dest, evalptr);
      return f;
    }
  }
  
  
  
  // Template for a member action
  template<typename F, typename W, typename R, typename T, typename... As>
  struct member_action_impl_t: public action_base<F> {
    R operator()(const client<T>& obj, As... args) const
    {
      return (*obj.*W::get_value())(args...);
    }
    typedef action_evaluate<F, R, client<T>, As...> evaluate;
    typedef action_finish<F, R> finish;
  };
  
  // Get the member action implementation (with its template
  // arguments) for a member function wrapper
  template<typename F, typename W, typename R, typename T, typename... As>
  member_action_impl_t<F, W, R, T, As...>
  get_member_action_impl_t(R(T::*)(As...));
  template<typename F, typename W, typename R, typename T, typename... As>
  member_action_impl_t<F, W, R, T, As...>
  get_const_member_action_impl_t(R(T::*)(As...)const);
  
  // TODO: don't expect a wrapper, expect the function instead
  // TODO: determine the function's type automatically
  template<typename F, typename W>
  using member_action_impl =
    decltype(get_member_action_impl_t<F, W>(W::get_value()));
  template<typename F, typename W>
  using const_member_action_impl =
    decltype(get_const_member_action_impl_t<F, W>(W::get_value()));
  
  
  
  // Call a member action via a client
  
  // Whether a type is a global type, and thus provides get_proc()
  template<typename T>
  struct is_global_helper: std::false_type {};
  template<typename T>
  struct is_global_helper<global_ptr<T> >: std::true_type {};
  template<typename T>
  struct is_global_helper<global_shared_ptr<T> >: std::true_type {};
  template<typename T>
  struct is_global_helper<client<T> >: std::true_type {};
  template<typename T>
  struct is_global:
    is_global_helper<typename std::remove_cv
                     <typename std::remove_reference<T>::type>::type> {};
  
  template<typename F, typename G, typename... As>
  auto sync(remote policy, const F& func, G&& global, As&&... args) ->
    typename enable_if<(is_action<F>::value && is_global<G>::value),
                       typename invoke_of<F, G, As...>::type>::type
  {
    RPC_ASSERT((policy & remote::sync) != remote(0));
    RPC_ASSERT((policy & ~remote::sync) == remote(0));
    int dest = global.get_proc();
    return sync(policy, dest,
                F(), std::forward<G>(global), std::forward<As>(args)...);
  }
  
  template<typename F, typename G, typename... As>
  auto detached(remote policy, const F& func, G&& global, As&&... args) ->
  typename enable_if<(is_action<F>::value && is_global<G>::value),
                     void>::type
  {
    RPC_ASSERT(policy == remote::detached);
    typedef typename invoke_of<F, As...>::type R;
    promise<R>* p = nullptr;
    auto evalptr = make_shared<typename F::evaluate>(p,
                                                     std::forward<G>(global),
                                                     std::forward<As>(args)...);
    thread([evalptr]() {
        int dest = std::get<0>(evalptr->args)->get_proc();
#ifndef RPC_DISABLE_CALL_SHORTCUT
        if (dest == server->rank()) {
          tuple_apply(F(), evalptr->args);
          return;
        }
#endif
        server->call(dest, evalptr);
      }).detach();
  }
  
  template<typename F, typename G, typename... As>
  auto async(remote policy, const F& func, G&& global, As&&... args) ->
  typename enable_if<(is_action<F>::value && is_global<G>::value),
                     future<typename invoke_of<F, G, As...>::type> >::type
  {
    RPC_ASSERT((policy & (remote::async | remote::deferred)) != remote(0));
    RPC_ASSERT((policy & ~(remote::async | remote::deferred)) == remote(0));
    bool is_deferred = (policy & remote::deferred) == remote::deferred;
    launch lpolicy = is_deferred ? launch::deferred : launch::async;
    typedef typename invoke_of<F, G, As...>::type R;
    auto p = new promise<R>;
    auto evalptr = make_shared<typename F::evaluate>(p,
                                                     std::forward<G>(global),
                                                     std::forward<As>(args)...);
    return async(lpolicy,
                 [evalptr]() {
                   int dest = std::get<0>(evalptr->args).get_proc();
#ifndef RPC_DISABLE_CALL_SHORTCUT
                   if (dest == server->rank()) {
                     delete evalptr->p.get();
                     return tuple_apply(F(), evalptr->args);
                   }
#endif
                   auto f = evalptr->p->get_future();
                   server->call(dest, evalptr);
                   return f.get();
                 });
  }
  
}

#define RPC_CALL_HH_DONE
#else
#  ifndef RPC_CALL_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CALL
