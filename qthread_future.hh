#ifndef QTHREAD_FUTURE_HH
#define QTHREAD_FUTURE_HH

#include <qthread/qthread.hpp>

#include "rpc_tuple.hh"

#include "cxx_utils.hh"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>



namespace qthread {
  
  template<typename T>
  class future;
  template<typename T>
  class shared_future;
  template<typename T>
  class promise;
  
  namespace detail {
    // TODO: remove these
    template<typename T>
    struct is_future;
    template<typename T>
    struct is_future<future<T> > { static constexpr bool value = true; };
    template<typename T>
    struct is_future<shared_future<T> > { static constexpr bool value = true; };
    
    template<typename T>
    struct future_traits;
    template<typename T>
    struct future_traits<future<T> >
    {
      static constexpr bool value = true;
      typedef T value_type;
    };
    template<typename T>
    struct future_traits<shared_future<T> >
    {
      static constexpr bool value = true;
      typedef T value_type;
    };
  }
  
  template<typename T>
  class future_state {
    mutable syncvar m_ready;
    bool has_exception;
    bool is_destructed;         // TODO
    int magic;
    T value;
  public:
    future_state(): has_exception(false)
                  , is_destructed(false), magic(0)
 { m_ready.empty(); }
    ~future_state() {
      RPC_ASSERT(!is_destructed);
      is_destructed=true;
      magic = 0x1234;
 m_ready.fill(); }
    future_state(const future_state&) = delete;
    future_state(future_state&&) = delete;
    future_state& operator=(const future_state&) = delete;
    future_state& operator=(future_state&&) = delete;
    bool ready() const { return m_ready.status(); }
    void wait() const
    {
      RPC_ASSERT(!is_destructed);
      m_ready.readFF();
      RPC_ASSERT(ready());
    }
    void set_value(const T& value_)
    {
      RPC_ASSERT(!is_destructed);
      RPC_ASSERT(!ready());
      value = value_;
      m_ready.fill();
      RPC_ASSERT(ready());
    }
    void set_value(T&& value_)
    {
      RPC_ASSERT(!is_destructed);
      RPC_ASSERT(!ready());
      std::swap(value, value_);
      m_ready.fill();
      RPC_ASSERT(ready());
    }
    void set_exception()
    {
      RPC_ASSERT(!is_destructed);
      RPC_ASSERT(!ready());
      has_exception = true;
      RPC_ASSERT(0);       // TODO
      m_ready.fill();
    }
    T& get_value()
    {
      RPC_ASSERT(!is_destructed);
      wait();
      RPC_ASSERT(!has_exception);
      return value;
    }
  };
  
  template<>
  class future_state<void> {
    mutable syncvar m_ready;
    bool has_exception;
  public:
    future_state(): has_exception(false) { m_ready.empty(); }
    ~future_state() { m_ready.fill(); }
    future_state(const future_state&) = delete;
    future_state(future_state&&) = delete;
    future_state& operator=(const future_state&) = delete;
    future_state& operator=(future_state&&) = delete;
    bool ready() const { return m_ready.status(); }
    void wait() const
    {
      m_ready.readFF();
      RPC_ASSERT(ready());
    }
    void set_value()
    {
      RPC_ASSERT(!ready());
      m_ready.fill();
      RPC_ASSERT(ready());
    }
    void set_exception()
    {
      RPC_ASSERT(!ready());
      has_exception = true;
      RPC_ASSERT(0);       // TODO
      m_ready.fill();
    }
    void get_value()
    {
      wait();
      RPC_ASSERT(!has_exception);
    }
  };
  
  
  
  template<typename T>
  class shared_future {
    typedef T value_type;
    std::shared_ptr<future_state<value_type> > state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<value_type> >& state):
      state(state)
    {
    }
    friend class future<value_type>;
    friend class promise<value_type>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<value_type>&& other);
    shared_future(const shared_future& other): state(other.state) {}
    shared_future(shared_future&& other): shared_future() { swap(other); }
    ~shared_future() {
      state=nullptr;            // TODO
}
    shared_future& operator=(const shared_future& other)
    {
      state = other.state;
      return *this;
    }
    shared_future& operator=(shared_future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    const value_type& get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(const F& func) const ->
      future<typename rpc::invoke_of<F, const shared_future&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: move func instead of copying it
      // TODO: optimize this
      return async([](const F& func, const shared_future& f) {
          f.wait(); return rpc::invoke(func, f);
        }, func, *this);
    }
    template<typename U>
    typename std::enable_if<(std::is_same<U, T>::value &&
                             detail::is_future<U>::value),
                            shared_future<typename U::value_type> >::type
    unwrap() const
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return then([](const shared_future& f) { return f.get().get(); });
    }
    bool ready() const { return state->ready(); }
  };
  
  template<typename T>
  class shared_future<T&> {
    typedef T& value_type;
    std::shared_ptr<future_state<T> > state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<value_type> >& state):
      state(state)
    {
    }
    friend class future<value_type>;
    friend class promise<value_type>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<value_type>&& other);
    shared_future(const shared_future& other): state(other.state) {}
    shared_future(shared_future&& other): shared_future() { swap(other); }
    ~shared_future() {}
    shared_future& operator=(const shared_future& other)
    {
      state = other.state;
      return *this;
    }
    shared_future& operator=(shared_future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    const value_type get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(const F& func) const ->
      future<typename rpc::invoke_of<F, const shared_future&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return async([](const F& func, const shared_future& f) {
          f.wait(); return rpc::invoke(func, f);
        }, func, *this);
    }
    bool ready() const { return state->ready(); }
  };
  
  template<>
  class shared_future<void> {
    typedef void value_type;
    std::shared_ptr<future_state<value_type> > state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<value_type> >& state):
      state(state)
    {
    }
    friend class future<value_type>;
    friend class promise<value_type>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<value_type>&& other);
    shared_future(const shared_future& other): state(other.state) {}
    shared_future(shared_future&& other): shared_future() { swap(other); }
    ~shared_future() {}
    shared_future& operator=(const shared_future& other)
    {
      state = other.state;
      return *this;
    }
    shared_future& operator=(shared_future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    value_type get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(const F& func) const ->
      future<typename rpc::invoke_of<F, const shared_future&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return async([](const F& func, const shared_future& f) {
          f.wait(); return rpc::invoke(func, f);
        }, func, *this);
    }
    bool ready() const { return state->ready(); }
  };
  
  
  
  template<typename T>
  class future {
    typedef T value_type;
    std::shared_ptr<future_state<value_type> > state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<value_type> >& state): state(state)
    {
    }
    friend class shared_future<value_type>;
    friend class promise<value_type>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {
      state=nullptr;            // TODO
}
    future& operator=(future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<value_type> share()
    {
      return shared_future<value_type>(std::move(*this));
    }
    value_type get()
    {
      auto value = state->get_value();
      state = nullptr;
      return value;
    };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(const F& func) ->
      future<typename rpc::invoke_of<F, future&&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return async([](const F& func, future&& f) {
          f.wait(); return rpc::invoke(func, std::move(f));
        }, std::forward<F>(func), std::move(*this));
    }
    template<typename U>
    typename std::enable_if<(std::is_same<U, T>::value &&
                             detail::is_future<U>::value),
                            future<typename U::value_type> >::type
    unwrap()
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return then([](future&& f) {
          return f.get().get();
        });
    }
    bool ready() const { return state->ready(); }
  };
  
  template<typename T>
  class future<T&> {
    typedef T& value_type;
    std::shared_ptr<future_state<value_type> > state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<T> >& state): state(state) {}
    friend class shared_future<value_type>;
    friend class promise<value_type>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {}
    future& operator=(future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<value_type> share()
    {
      return shared_future<T>(std::move(*this));
    }
    value_type get() { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(F&& func) -> future<typename rpc::invoke_of<F, future&&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return async([func](future&& f) { f.wait(); return func(std::move(f)); }, std::move(*this));
    }
    bool ready() const { return state->ready(); }
  };
  
  template<>
  class future<void> {
    typedef void value_type;
    std::shared_ptr<future_state<value_type> > state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<value_type> >& state): state(state)
    {
    }
    friend class shared_future<value_type>;
    friend class promise<value_type>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {}
    future& operator=(future&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<value_type> share()
    {
      return shared_future<value_type>(std::move(*this));
    }
    value_type get() { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
    template<typename F>
    auto then(F&& func) -> future<typename rpc::invoke_of<F, future&&>::type>
    {
      RPC_ASSERT(valid());
      // TODO: optimize this
      return async([func](future&& f) { f.wait(); return func(std::move(f)); }, std::move(*this));
    }
    bool ready() const { return state->ready(); }
  };
  
  template<typename T>
  shared_future<T>::shared_future(future<T>&& other):
    state(nullptr)
  {
    std::swap(state, other.state);
  }
  
  // inline shared_future<void>::shared_future(future<void>&& other):
  //   state(nullptr)
  // {
  //   std::swap(state, other.state);
  // }
  
  
  
  template<typename T>
  class promise {
    std::shared_ptr<future_state<T> > state;
  public:
    promise(): state(std::make_shared<future_state<T> >()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->ready()) set_exception(); }
    promise& operator=(promise&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    // TODO: allow only one call to get_future
    future<T> get_future() { RPC_ASSERT(state!=nullptr); return future<T>(state); }
    void set_value(const T& value) { state->set_value(value); }
    void set_value(T&& value) { state->set_value(std::forward<T>(value)); }
    void set_exception() { state->set_exception(); }
  };
  
  template<typename T>
  class promise<T&> {
    std::shared_ptr<future_state<T> > state;
  public:
    promise(): state(std::make_shared<future_state<T&> >()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->ready()) set_exception(); }
    promise& operator=(promise&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    // TODO: allow only one call to get_future
    future<T&> get_future() { RPC_ASSERT(state); return future<T&>(state); }
    void set_value(T& value) { state->set_value(value); }
    void set_exception() { state->set_exception(); }
  };
  
  template<>
  class promise<void> {
    std::shared_ptr<future_state<void> > state;
  public:
    promise(): state(std::make_shared<future_state<void> >()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->ready()) set_exception(); }
    promise& operator=(promise&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    future<void> get_future() { return future<void>(state); }
    void set_value() { state->set_value(); }
    void set_exception() { state->set_exception(); }
  };
  
  template<typename T>
  void swap(promise<T>& lhs, promise<T>& rhs)
  {
    lhs.swap(rhs);
  }
  
  
  
  template<typename T> 
  future<typename std::decay<T>::type> make_future(T&& value) 
  {
    promise<typename std::decay<T>::type> p;
    p.set_value(std::forward<T>(value));
    return p.get_future();
  }
  
  inline future<void> make_future() 
  {
    promise<void> p;
    p.set_value();
    return p.get_future();
  }
  
  template<typename T> 
  shared_future<typename std::decay<T>::type> make_shared_future(T&& value) 
  {
    return make_future(std::forward<T>(value)).share();
  }
  
  inline shared_future<void> make_shared_future() 
  {
    return make_future().share();
  }
  
}

#define QTHREAD_FUTURE_HH_DONE
#else
#  ifndef QTHREAD_FUTURE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_FUTURE_HH
