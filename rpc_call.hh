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
#include <mutex>
#include <sstream>

namespace rpc {
  
  using std::bind;
  using std::condition_variable;
  using std::enable_if;
  using std::future;
  using std::is_void;
  using std::istringstream;
  using std::make_shared;
  using std::mutex;
  using std::ostringstream;
  using std::promise;
  using std::shared_future;
  using std::shared_ptr;
  using std::string;
  using std::unique_lock;
  
  
  
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
      action_finish<F, R>(p, tuple_map<F, As...>(F(), args))();
#else
      auto cont = action_finish<F, R>(p, tuple_map<F, As...>(F(), args));
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
      (action_finish<F, void>(p))();
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
  
  
  
  // Could also use thread.detach instead of async
  
  template<typename F, typename... As>
  void apply(const F& func, As... args)
  {
    // TODO: omit continuation
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    std::async(action_evaluate<F, R, As...>(p, args...));
  }
  template<typename F, typename... As>
  auto async(const F& func, As... args) -> future<decltype(func(args...))>
  {
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
    std::async(action_evaluate<F, R, As...>(p, args...));
    return f;
  }
  template<typename F, typename... As>
  auto sync(const F& func, As... args) -> decltype(func(args...))
  {
    typedef decltype(func(args...)) R;
    auto p = new promise<R>;
    auto f = p->get_future();
#if 0
    action_evaluate<F, R, As...>(p, args...)();
#else
    auto cont = action_evaluate<F, R, As...>(p, args...);
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
