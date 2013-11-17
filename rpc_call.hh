#ifndef RPC_CALL
#define RPC_CALL

#include "rpc_defs.hh"

#include <cassert>
#include <future>
#include <memory>
#include <mutex>

namespace rpc {
  
  using std::condition_variable;
  using std::enable_if;
  using std::is_void;
  using std::make_shared;
  using std::mutex;
  // using std::shared_future;
  using std::shared_ptr;
  using std::unique_lock;
  
  
  
  // Futures, re-implemented
  
  template<typename T>
  class shared_future {
    struct state_t {
      T value;
      bool ready;
      // std::shared_future<void> f;
      mutex mtx;
      condition_variable cv;
      state_t(): ready(false) {}
      // void remember_thread(const std::shared_future<void>& f_) { f=f_; }
      void set(const T& value_)
      {
        assert(!ready);
        unique_lock<mutex> lck(mtx);
        value = value_;
        ready = true;
        cv.notify_all();
      }
      T& get()
      {
        unique_lock<mutex> lck(mtx);
        while (!ready) cv.wait(lck);
        return value;
      }
    };
    shared_ptr<state_t> state;
  public:
    shared_future(): state(make_shared<state_t>()) {}
    // void remember_thread(const std::shared_future<void>& f) const
    // {
    //   state->remember_thread(f);
    // }
    void set(const T& value) const { state->set(value); }
    T& get() const { return state->get(); }
  };
  
  template<>
  class shared_future<void> {
    struct state_t {
      bool ready;
      // std::shared_future<void> f;
      mutex mtx;
      condition_variable cv;
      state_t(): ready(false) {}
      // void remember_thread(const std::shared_future<void>& f_) { f=f_; }
      void set()
      {
        assert(!ready);
        unique_lock<mutex> lck(mtx);
        ready = true;
        cv.notify_all();
      }
      void get()
      {
        unique_lock<mutex> lck(mtx);
        while (!ready) cv.wait(lck);
      }
    };
    shared_ptr<state_t> state;
  public:
    shared_future(): state(make_shared<state_t>()) {}
    // void remember_thread(const std::shared_future<void>& f) const
    // {
    //   state->remember_thread(f);
    // }
    void set() const { state->set(); }
    void get() const { return state->get(); }
  };
  
  
  
  template<typename R>
  struct action_finish {
    void operator()(const shared_future<R>& f, R res) const
    {
      f.set(res);
    }
  };
  template<>
  struct action_finish<void> {
    void operator()(const shared_future<void>& f) const
    {
      f.set();
    }
  };
  
  template<typename action, typename R, typename... As>
  struct action_evaluate {
    void operator()(const action_finish<R>& cont,
                    const shared_future<R>& f,
                    As... args)
      const
    {
      cont(f, action()(args...));
    }
  };
  template<typename action, typename... As>
  struct action_evaluate<action, void, As...> {
    void operator()(const action_finish<void>& cont,
                    const shared_future<void>& f,
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
      shared_future<R> f;
      // f.remember_thread(async(evaluate(), action_finish(), f, args...));
      std::async(action_evaluate<action, R, As...>(),
                 action_finish<R>(), f,
                 args...);
      return f;
    }
  };
  
  template<typename action>
  struct action_base {
  };
  
  
  
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
