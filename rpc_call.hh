#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_defs.hh"
#include "rpc_global_ptr.hh"

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
  using std::cout;
  using std::enable_if;
  using std::flush;
  using std::is_void;
  using std::istringstream;
  using std::make_shared;
  using std::mutex;
  using std::ostringstream;
  using std::shared_future;
  using std::shared_ptr;
  using std::string;
  using std::unique_lock;
  
  
  
  // Futures, re-implemented
  
  template<typename T>
  class rpc_future {
    T value;
    bool ready;
    // shared_future<void> f;
    mutable mutex mtx;
    mutable condition_variable cv;
  public:
    rpc_future(): ready(false) {}
    // void remember_thread(const shared_future<void>& f_) { f=f_; }
    void set(const T& value_)
    {
      assert(!ready);
      cout << "rpc_future.set.0\n";
      unique_lock<mutex> lck(mtx);
      value = value_;
      ready = true;
      cv.notify_all();
      cout << "rpc_future.set.1\n";
    }
    T get() const
    {
      unique_lock<mutex> lck(mtx);
      cout << "rpc_future.get.0\n";
      while (!ready) cv.wait(lck);
      cout << "rpc_future.get.1\n";
      return value;
    }
  };
  
  template<>
  class rpc_future<void> {
    bool ready;
    // shared_future<void> f;
    mutable mutex mtx;
    mutable condition_variable cv;
  public:
    rpc_future(): ready(false) {}
    // void remember_thread(const shared_future<void>& f_) { f=f_; }
    void set()
    {
      cout << "rpc_future.set.0\n";
      assert(!ready);
      unique_lock<mutex> lck(mtx);
      ready = true;
      cv.notify_all();
      cout << "rpc_future.set.1\n";
    }
    void get() const
    {
      unique_lock<mutex> lck(mtx);
      cout << "rpc_future.get.0\n";
      while (!ready) cv.wait(lck);
      cout << "rpc_future.get.1\n";
    }
  };
  
  
  
  // Base class for all callable RPC objects
  struct callable_base {
    virtual ~callable_base() {}
    virtual void operator()() const = 0;
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      cout << "callable_base.serialize.0\n";
    }
  };
// }
// BOOST_CLASS_EXPORT(rpc::callable_base);
// namespace rpc{
  
  struct callable_ptr {
    callable_base* ptr;
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      cout << "callable_ptr.serialize.0\n" << flush;
      ar & ptr;
      cout << "callable_ptr.serialize.1\n" << flush;
    }
  };
  
  
  
#if 0
  
  template<typename R>
  struct action_finish {
    void operator()(rpc_future<R>* f, R res) const { f->set(res); }
  };
  template<>
  struct action_finish<void> {
    void operator()(rpc_future<void>* f) const { f->set(); }
  };
  
  template<typename action, typename R, typename... As>
  struct action_evaluate {
    void operator()(const action_finish<R>& cont,
                    rpc_future<R>* f,
                    As... args)
      const
    {
      cont(f, action()(args...));
    }
  };
  template<typename action, typename... As>
  struct action_evaluate<action, void, As...> {
    void operator()(const action_finish<void>& cont,
                    rpc_future<void>* f,
                    As... args)
      const
    {
      action()(args...);
      cont(f);
    }
  };
  
  template<typename action, typename R, typename... As>
  struct action_call {
    shared_future<R> operator()(As... args) const
    {
      auto f = make_shared<rpc_future<R>>();
      // f->remember_thread(async(evaluate(), action_finish(), f, args...));
      std::async(action_evaluate<action, R, As...>(),
                 action_finish<R>(), f.get(),
                 args...);
      // Object lifetime: The lambda expression below will block until
      // the continuation has set the value; at this time, the
      // rpc_future is not needed any more. Once the lambda expression
      // returns, the shared_ptr will destruct the rpc_future.
      return std::async([=](){ return f->get(); });
    }
  };
  
#else
  
  template<typename R>
  struct action_finish: public callable_base {
    global_ptr<rpc_future<R>> f;
    R res;
    action_finish(global_ptr<rpc_future<R>> f, R res): f(f), res(res) {}
    void operator()() const { f.get()->set(res); }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & f & res;
    }
  };
  template<>
  struct action_finish<void>: public callable_base {
    rpc_future<void>* f;
    action_finish(rpc_future<void>* f): f(f) {}
    void operator()() const { f->set(); }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & f;
    }
  };
  
  template<typename action, typename R, typename... As>
  struct action_evaluate: public callable_base {
    global_ptr<rpc_future<R>> f;
    tuple<As...> args;
    action_evaluate(rpc_future<R>* f, As... args): f(f), args(args...) {}
    void operator()() const
    {
#if 1
      cout << "action_evaluate.0\n";
      action_finish<R>(f, tuple_map<action, As...>(action(), args))();
      cout << "action_evaluate.1\n";
#elif 0
      auto cont = action_finish<R>(f, tuple_map<action, As...>(action(), args));
      cont();
#else
      cout << "action_evaluate.0\n";
      auto cont = action_finish<R>(f, tuple_map<action, As...>(action(), args));
      cout << "action_evaluate.1\n";
      callable_base* callable = &cont;
      cout << "action_evaluate.2\n";
      ostringstream os;
      cout << "action_evaluate.3\n";
      boost::archive::text_oarchive ar(os);
      cout << "action_evaluate.4\n";
      // ar << callable;
      callable_ptr ptr;
      ptr.ptr = &cont;
      ar << ptr;
      cout << "action_evaluate.5\n";
      string call = os.str();
      cout << "action_evaluate.6\n";
      cout << "[archived call: " << call << "]\n";
      cout << "action_evaluate.7\n";
      istringstream is(call);
      cout << "action_evaluate.8\n";
      boost::archive::text_iarchive ar1(is);
      cout << "action_evaluate.9\n";
      callable_base* callable1;
      cout << "action_evaluate.10\n";
      ar1 >> callable1;
      cout << "action_evaluate.11\n";
      callable1->operator()();
      cout << "action_evaluate.12\n";
      // (*callable1)();
      cout << "action_evaluate.13\n";
#endif
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      cout << "action_evaluate.serialize.0\n";
      ar & boost::serialization::base_object<callable_base>(*this);
      cout << "action_evaluate.serialize.1\n";
      ar & f & args;
      cout << "action_evaluate.serialize.2\n";
    }
  };
  template<typename action, typename... As>
  struct action_evaluate<action, void, As...>: public callable_base {
    rpc_future<void>* f;
    tuple<As...> args;
    action_evaluate(rpc_future<void>* f, As... args): f(f), args(args...) {}
    void operator()() const
    {
      tuple_map<action, As...>(action(), args);
#if 1
      (action_finish<void>(f))();
#else
      auto cont = action_finish<void>(f);
      cont();
#endif
    }
  private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive& ar, unsigned int file_version)
    {
      ar & boost::serialization::base_object<callable_base>(*this);
      ar & f & args;
    }
  };
  
  template<typename action, typename R, typename... As>
  struct action_call {
    shared_future<R> operator()(As... args) const
    {
      auto f = make_shared<rpc_future<R>>();
      // f->remember_thread(async(evaluate(), f, args...));
      std::async(action_evaluate<action, R, As...>(f.get(), args...));
      // Object lifetime: The lambda expression below will block until
      // the continuation has set the value; at this time, the
      // rpc_future is not needed any more. Once the lambda expression
      // returns, the shared_ptr will destruct the rpc_future.
      return std::async([=](){ return f->get(); });
    }
  };
  
#endif
  
  
  
  // TODO: use enable_if to ensure func is derived from action_base
  
  template<typename F, typename... As>
  void apply(const F& func, As... args)
  {
    // TODO: omit continuation
    typedef decltype(func(args...)) R;
    action_call<F, R, As...>()(args...).get();
  }
  template<typename F, typename... As>
  auto async(const F& func, As... args) ->
    shared_future<decltype(func(args...))>
  {
    typedef decltype(func(args...)) R;
    return action_call<F, R, As...>()(args...);
  }
  template<typename F, typename... As>
  auto sync(const F& func, As... args) -> decltype(func(args...))
  {
    typedef decltype(func(args...)) R;
    // return action_call<F, R, As...>()(args...).get();
    cout << "sync.0\n";
    auto f = make_shared<rpc_future<R>>();
    cout << "sync.1\n";
    action_evaluate<F, R, As...>(f.get(), args...)();
    cout << "sync.2\n";
    return f.get()->get();
  }
  
}

#endif  // #ifndef RPC_CALL
