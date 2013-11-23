#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_global_ptr.hh"
#include "rpc_server.hh"
#include "rpc_tuple.hh"

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
#include <future>
#include <sstream>
#include <thread>
#include <type_traits>

namespace rpc {
  
  using boost::make_shared;
  using boost::shared_ptr;
  
  using std::enable_if;
  using std::future;
  using std::future_status;
  using std::is_base_of;
  using std::istringstream;
  using std::ostringstream;
  using std::promise;
  using std::string;
  using std::thread;
  
  
  
  // Base class for all callable RPC objects
  struct callable_base {
    virtual ~callable_base() {}
    virtual void operator()() = 0;
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version) {}
  };
  
  
  
  template<typename F, typename R>
  struct action_finish: public callable_base {
    global_ptr<promise<R>> p;
    R res;
    action_finish() {}          // only for boost::serialize
    action_finish(const global_ptr<promise<R>>& p, R res): p(p), res(res) {}
    void operator()()
    {
      p.get()->set_value(res);
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
    global_ptr<promise<void>> p;
    action_finish() {}          // only for boost::serialize
    action_finish(const global_ptr<promise<void>>& p): p(p) {}
    void operator()()
    {
      p.get()->set_value();
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
    global_ptr<promise<R>> p;
    tuple<As...> args;
    action_evaluate() {}        // only for boost::serialize
    action_evaluate(global_ptr<promise<R>> p, As... args):
      p(p), args(args...) {}
    void operator()()
    {
      auto cont = tuple_map<F, As...>(F(), args);
      if (p.is_empty()) return;
      // typename F::finish(p, cont)();
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
    global_ptr<promise<void>> p;
    tuple<As...> args;
    action_evaluate() {}        // only for boost::serialize
    action_evaluate(global_ptr<promise<void>> p, As... args):
      p(p), args(args...) {}
    void operator()()
    {
      tuple_map<F, As...>(F(), args);
      if (p.is_empty()) return;
      // (typename F::finish(p))();
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
  
  // Wrapper for a function, turning a function into a type
  template<typename FT, const FT& FV>
  struct wrap {
    static constexpr FT& value = FV;
  };
  
  // Determine the return type of a function
  // TODO: move to a more generic place
  template<typename T>
  struct return_type;
  template<typename R, typename... As>
  struct return_type<R (*)(As...)>
  {
    typedef R type;
  };
  
  // Template for an action
  template<typename F, typename W, typename... As>
  struct action_impl_t: public action_base<F> {
    // typedef decltype(W::value(As()...)) R;
    typedef typename return_type<decltype(&W::value)>::type R;
    R operator()(As... args) const { return W::value(args...); }
    typedef action_evaluate<F, R, As...> evaluate;
    typedef action_finish<F, R> finish;
  };
  
  // Get the action implementation (with its template arguments) for a
  // function wrapper
  template<typename F, typename W, typename R, typename... As>
  action_impl_t<F, W, As...> get_action_impl_t(R(As...));
  
  // TODO: don't expect a wrapper, expect the function instead
  // TODO: determine the function's type automatically
  template<typename F, typename W>
  using action_impl = decltype(get_action_impl_t<F, W>(W::value));
  
  // Example action definition for a given function "f":
  // struct f_action:
  //   public rpc::action_impl<f_action, rpc::wrap<decltype(f), f>>
  // {
  // };
  // BOOST_CLASS_EXPORT(f_action::evaluate);
  // BOOST_CLASS_EXPORT(f_action::finish);
  
  
  
  template<typename F, typename... As>
  auto apply(int dest, const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value, void>::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return thread(func, args...).detach();
    }
#endif
    typedef decltype(func(args...)) R;
    auto p = global_ptr<promise<R>>();
    server->call(dest, make_shared<typename F::evaluate>(p, args...));
  }
  template<typename F, typename... As>
  auto async(int dest, const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value,
                       future<decltype(func(args...))>>::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return std::async(func, args...);
    }
#endif
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
    server->call(dest, make_shared<typename F::evaluate>(p, args...));
    return f;
  }
  template<typename F, typename... As>
  auto sync(int dest, const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value,
                       decltype(func(args...))>::type
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (dest == server->rank()) {
      return func(args...);
    }
#endif
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
    server->call(dest, make_shared<typename F::evaluate>(p, args...));
    return f.get();
  }
  
}

#endif  // #ifndef RPC_CALL
