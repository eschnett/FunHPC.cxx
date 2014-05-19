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
  
  template<typename F>
  struct member_action_base: action_base<F> {
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
  
  
  
  // Call an action on a given destination
  
  // Whether a type is a (non-member) action
  template<typename T>
  using is_action = is_base_of<action_base<T>, T>;
  
  template<typename F, typename... As>
  auto detached(int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value, void>::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return thread(func, std::forward<As>(args)...).detach();
    }
#endif
    typedef typename invoke_of<F, As...>::type R;
    promise<R>* p = nullptr;
    server->call
      (dest, make_shared<typename F::evaluate>(p, std::forward<As>(args)...));
  }
  
  template<typename F, typename... As>
  auto async(int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value,
                       future<typename invoke_of<F, As...>::type> >::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return async(func, std::forward<As>(args)...);
    }
#endif
    typedef typename invoke_of<F, As...>::type R;
    auto p = new promise<R>;
    auto f = p->get_future();
    server->call
      (dest, make_shared<typename F::evaluate>(p, std::forward<As>(args)...));
    return f;
  }
  
  template<typename F, typename... As>
  auto sync(int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value,
                       typename invoke_of<F, As...>::type>::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return func(std::forward<As>(args)...);
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
  auto deferred(int dest, const F& func, As&&... args) ->
    typename enable_if<is_action<F>::value,
                       future<typename invoke_of<F, As...>::type> >::type
  {
    return async(launch::deferred,
                 [](int dest, const F& func, As&&... args) {
                   return sync(dest, func, std::forward<As>(args)...);
                 }, dest, func, std::forward<As>(args)...);
  }
  
  
  
  // Template for a member action
  template<typename F, typename W, typename R, typename T, typename... As>
  struct member_action_impl_t: public member_action_base<F> {
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
  
  // Whether a type is a member action
  template<typename T>
  using is_member_action = is_base_of<member_action_base<T>, T>;
  
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
  auto deferred(const F& func, G&& global, As&&... args) ->
    typename enable_if<(is_member_action<F>::value && is_global<G>::value),
                       future<typename invoke_of<F, G, As...>::type> >::type
  {
    return async(launch::deferred,
                 [](const F& func, G&& global, As&&... args) {
                   int proc = global.get_proc();
                   return sync(proc, func, std::forward<G>(global),
                               std::forward<As>(args)...);
                 }, func, std::forward<G>(global), std::forward<As>(args)...);
  }
  
  template<typename F, typename G, typename... As>
  auto detached(const F& func, G&& global, As&&... args) ->
    typename enable_if<(is_member_action<F>::value && is_global<G>::value),
                       void>::type
  {
    return
      thread([](const F& func, G&& global, As&&... args) {
          int proc = global.get_proc();
          return sync(proc, func, std::forward<G>(global),
                      std::forward<As>(args)...);
        }, func, std::forward<G>(global), std::forward<As>(args)...).detach();
  }
  
  template<typename F, typename G, typename... As>
  auto async(const F& func, G&& global, As&&... args) ->
    typename enable_if<(is_member_action<F>::value && is_global<G>::value),
                       future<typename invoke_of<F, G, As...>::type> >::type
  {
    return async([](const F& func, G&& global, As&&... args) ->
                 typename invoke_of<F, G, As...>::type
                 {
                   int proc = global.get_proc();
                   return sync(proc, func, std::forward<G>(global),
                               std::forward<As>(args)...);
                 }, func, std::forward<G>(global), std::forward<As>(args)...);
  }
  
  template<typename F, typename G, typename... As>
  auto sync(const F& func, G&& global, As&&... args) ->
    typename enable_if<(is_member_action<F>::value && is_global<G>::value),
                       typename invoke_of<F, G, As...>::type>::type
  {
    int proc = global.get_proc();
    return sync(proc, func, std::forward<G>(global), std::forward<As>(args)...);
  }
  
}

#define RPC_CALL_HH_DONE
#else
#  ifndef RPC_CALL_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_CALL
