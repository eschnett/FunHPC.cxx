#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_defs.hh"

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace rpc {
  
  using std::bind;
  using std::condition_variable;
  using std::enable_if;
  using std::is_void;
  using std::make_shared;
  using std::mutex;
  using std::shared_future;
  using std::shared_ptr;
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
      unique_lock<mutex> lck(mtx);
      value = value_;
      ready = true;
      cv.notify_all();
    }
    T get() const
    {
      unique_lock<mutex> lck(mtx);
      while (!ready) cv.wait(lck);
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
      assert(!ready);
      unique_lock<mutex> lck(mtx);
      ready = true;
      cv.notify_all();
    }
    void get() const
    {
      unique_lock<mutex> lck(mtx);
      while (!ready) cv.wait(lck);
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
  struct action_finish {
    rpc_future<R>* f;
    R res;
    action_finish(rpc_future<R>* f, R res): f(f), res(res) {}
    void operator()() const { f->set(res); }
  };
  template<>
  struct action_finish<void> {
    rpc_future<void>* f;
    action_finish(rpc_future<void>* f): f(f) {}
    void operator()() const { f->set(); }
  };
  
  template<typename action, typename R, typename... As>
  struct action_evaluate {
    rpc_future<R>* f;
    tuple<As...> args;
    action_evaluate(rpc_future<R>* f, As... args): f(f), args(args...) {}
    void operator()() const
    {
      action_finish<R>(f, tuple_map<action, As...>(action(), args))();
    }
  };
  template<typename action, typename... As>
  struct action_evaluate<action, void, As...> {
    rpc_future<void>* f;
    tuple<As...> args;
    action_evaluate(rpc_future<void>* f, As... args): f(f), args(args...) {}
    void operator()() const
    {
      tuple_map<action, As...>(action(), args);
      (action_finish<void>(f))();
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
  
  
  
  template<typename action>
  struct action_base {
  };
  
  
  
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
    return action_call<F, R, As...>()(args...).get();
  }
  
}

#endif  // #ifndef RPC_CALL
