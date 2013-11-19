#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_defs.hh"
#include "rpc_global_ptr.hh"
#include "rpc_tuple.hh"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <sstream>
#include <type_traits>

namespace rpc {
  
  using std::enable_if;
  using std::future;
  using std::is_base_of;
  using std::istringstream;
  using std::make_shared;
  using std::ostringstream;
  using std::promise;
  using std::shared_ptr;
  using std::string;
  
  
  
  // Base class for all callable RPC objects
  struct callable_base {
    virtual ~callable_base() {}
    virtual void operator()() = 0;
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
    }
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
    action_evaluate(promise<R>* p, As... args): p(p), args(args...) {}
    void operator()()
    {
#if 0
      typename F::finish(p, tuple_map<F, As...>(F(), args))();
#else
      auto cont = typename F::finish(p, tuple_map<F, As...>(F(), args));
      callable_base* callable = &cont;
      ostringstream os;
      boost::archive::text_oarchive ar(os);
      ar << callable;
      string call = os.str();
      // std::cout << "[archived call: " << call << "]\n";
      istringstream is(call);
      boost::archive::text_iarchive ar1(is);
      callable_base* callable1;
      ar1 >> callable1;
      (*callable1)();
#endif
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
    action_evaluate(promise<void>* p, As... args): p(p), args(args...) {}
    void operator()()
    {
      tuple_map<F, As...>(F(), args);
      (typename F::finish(p))();
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
  
  // Example for a wrapper for a function
  // struct wrap_f {
  //   static constexpr decltype(&f) value = f;
  // };
  
  // Template for an action
  template<typename F, typename W, typename... As>
  struct action_impl_t: public action_base<F> {
    typedef decltype(W::value(As()...)) R;
    R operator()(As... args) const { return W::value(args...); }
    typedef action_evaluate<F, R, As...> evaluate;
    typedef action_finish<F, R> finish;
  };
  
  // Get the action implementation (with its template arguments) for a
  // function wrapper
  template<typename F, typename W, typename R, typename... As>
  action_impl_t<F, W, As...> get_action_impl_t(R(As...));
  
  template<typename F, typename W>
  struct action_impl {
    typedef decltype(get_action_impl_t<F, W>(W::value)) type;
  };
  
  // Example action definition
  // struct f_action: public rpc::action_impl<f_action, wrap_f>::type
  // {
  // };
  
  
  
  // Could also use thread.detach instead of async
  
  template<typename F, typename... As>
  auto apply(const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value, void>::type
  {
    // TODO: omit continuation
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    std::async(typename F::evaluate(p, args...));
  }
  template<typename F, typename... As>
  auto async(const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value,
                       future<decltype(func(args...))>>::type
  {
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
    std::async(typename F::evaluate(p, args...));
    return f;
  }
  template<typename F, typename... As>
  auto sync(const F& func, As... args) ->
    typename enable_if<is_base_of<action_base<F>, F>::value,
                       decltype(func(args...))>::type
  {
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
#if 0
    typename F::evaluate(p, args...)();
#else
    auto cont = typename F::evaluate(p, args...);
    callable_base* callable = &cont;
    ostringstream os;
    boost::archive::text_oarchive ar(os);
    ar << callable;
    string call = os.str();
    // std::cout << "[archived call: " << call << "]\n";
    istringstream is(call);
    boost::archive::text_iarchive ar1(is);
    callable_base* callable1;
    ar1 >> callable1;
    (*callable1)();
#endif
    return f.get();
  }
  
}

#endif  // #ifndef RPC_CALL
