#ifndef QTHREAD_FUTURE_HH
#define QTHREAD_FUTURE_HH

#include <qthread/qthread.hpp>

#include <memory>
#include <utility>



namespace qthread {
  
  template<typename T>
  class future;
  template<typename T>
  class shared_future;
  template<typename T>
  class promise;
  
  template<typename T>
  class future_state {
    mutable syncvar m_valid;
    bool has_exception;
    T value;
  public:
    future_state(): has_exception(false) { m_valid.empty(); }
    ~future_state() { m_valid.fill(); }
    future_state(const future_state&) = delete;
    future_state(future_state&&) = delete;
    future_state& operator=(const future_state&) = delete;
    future_state& operator=(future_state&&) = delete;
    bool valid() const { return m_valid.status(); }
    void wait() const
    {
      m_valid.readFF();
      assert(valid());
    }
    void set_value(const T& value_)
    {
      assert(!valid());
      value = value_;
      m_valid.fill();
      assert(valid());
    }
    void set_value(T&& value_)
    {
      assert(!valid());
      std::swap(value, value_);
      m_valid.fill();
      assert(valid());
    }
    void set_exception()
    {
      assert(!valid());
      has_exception = true;
      m_valid.fill();
    }
    T& get_value()
    {
      wait();
      assert(!has_exception);
      return value;
    }
  };
  
  template<>
  class future_state<void> {
    mutable syncvar m_valid;
    bool has_exception;
  public:
    future_state(): has_exception(false) { m_valid.empty(); }
    ~future_state() { m_valid.fill(); }
    future_state(const future_state&) = delete;
    future_state(future_state&&) = delete;
    future_state& operator=(const future_state&) = delete;
    future_state& operator=(future_state&&) = delete;
    bool valid() const { return m_valid.status(); }
    void wait() const
    {
      m_valid.readFF();
      assert(valid());
    }
    void set_value()
    {
      assert(!valid());
      m_valid.fill();
      assert(valid());
    }
    void set_exception()
    {
      assert(!valid());
      has_exception = true;
      m_valid.fill();
    }
    void get_value()
    {
      wait();
      assert(!has_exception);
    }
  };
  
  
  
  template<typename T>
  class shared_future {
    std::shared_ptr<future_state<T>> state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<T>>& state): state(state)
    {
    }
    friend class promise<T>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<T>&& other);
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
      swap(other);
      other.state.reset();
      return *this;
    }
    const T& get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<typename T>
  class shared_future<T&> {
    std::shared_ptr<future_state<T>> state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<T>>& state): state(state)
    {
    }
    friend class promise<T&>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<T&>&& other);
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
      swap(other);
      other.state.reset();
      return *this;
    }
    const T& get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<>
  class shared_future<void> {
    std::shared_ptr<future_state<void>> state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const std::shared_ptr<future_state<void>>& state):
      state(state)
    {
    }
    friend class promise<void>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<void>&& other);
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
      swap(other);
      other.state.reset();
      return *this;
    }
    void get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  
  
  template<typename T>
  class future {
    std::shared_ptr<future_state<T>> state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<T>>& state): state(state) {}
    friend class shared_future<T>;
    friend class promise<T>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {}
    future& operator=(future&& other)
    {
      swap(other);
      other.state.reset();
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<T> share() { return shared_future<T>(std::move(*this)); }
    T get() { auto value = state->get_value(); state = nullptr; return value; };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<typename T>
  class future<T&> {
    std::shared_ptr<future_state<T&>> state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<T>>& state): state(state) {}
    friend class shared_future<T&>;
    friend class promise<T&>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {}
    future& operator=(future&& other)
    {
      swap(other);
      other.state.reset();
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<T&> share() { return shared_future<T>(std::move(*this)); }
    T& get() { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<>
  class future<void> {
    std::shared_ptr<future_state<void>> state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const std::shared_ptr<future_state<void>>& state): state(state) {}
    friend class shared_future<void>;
    friend class promise<void>;
  public:
    future(): state(nullptr) {}
    future(future&& other): future() { swap(other); }
    future(const future& other) = delete;
    ~future() {}
    future& operator=(future&& other)
    {
      swap(other);
      other.state.reset();
      return *this;
    }
    future& operator=(const future& other) = delete;
    shared_future<void> share()
    {
      return shared_future<void>(std::move(*this));
    }
    void get() { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<typename T>
  shared_future<T>::shared_future(future<T>&& other):
    state(nullptr)
  {
    std::swap(state, other.state);
  }
  
  inline shared_future<void>::shared_future(future<void>&& other):
    state(nullptr)
  {
    std::swap(state, other.state);
  }
  
  template<typename T>
  class promise {
    std::shared_ptr<future_state<T>> state;
  public:
    promise(): state(std::make_shared<future_state<T>>()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->valid()) set_exception(); }
    promise& operator=(promise&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    // TODO: allow only one call to get_future
    future<T> get_future() { assert(state); return future<T>(state); }
    void set_value(const T& value) { state->set_value(value); }
    void set_value(T&& value) { state->set_value(std::forward<T>(value)); }
    void set_exception() { state->set_exception(); }
  };
  
  template<typename T>
  class promise<T&> {
    std::shared_ptr<future_state<T>> state;
  public:
    promise(): state(std::make_shared<future_state<T&>>()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->valid()) set_exception(); }
    promise& operator=(promise&& other)
    {
      state = nullptr;
      swap(other);
      return *this;
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    // TODO: allow only one call to get_future
    future<T&> get_future() { assert(state); return future<T&>(state); }
    void set_value(T& value) { state->set_value(value); }
    void set_exception() { state->set_exception(); }
  };
  
  template<>
  class promise<void> {
    std::shared_ptr<future_state<void>> state;
  public:
    promise(): state(std::make_shared<future_state<void>>()) {}
    promise(promise&& other): state(nullptr) { swap(other); }
    promise(const promise& other) = delete;
    ~promise() { if (state && !state->valid()) set_exception(); }
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
  future<typename std::decay<T>::type> make_ready_future(T&& value) 
  {
    promise<typename std::decay<T>::type> p;
    p.set_value(std::forward<T>(value));
    return p.get_future();
  }
  
  inline future<void> make_ready_future() 
  {
    promise<void> p;
    p.set_value();
    return p.get_future();
  }
  
}

#define QTHREAD_FUTURE_HH_DONE
#else
#  ifndef QTHREAD_FUTURE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_FUTURE_HH
