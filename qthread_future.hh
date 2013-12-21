#ifndef QTHREAD_FUTURE_HH
#define QTHREAD_FUTURE_HH

#include <qthread.h>

#include <memory>



namespace qthread {
  
  using std::make_shared;
  using std::move;
  using std::shared_ptr;
  
  
  
  template<typename T>
  class future;
  template<typename T>
  class shared_future;
  template<typename T>
  class promise;
  
  template<typename T>
  class future_state {
    aligned_t value_valid;
    T value;
  public:
    future_state(): value_valid(false) {}
    bool has_value() const { return value_valid; }
    void wait() const
    {
      if (!has_value()) {
        aligned_t tmp;
        qthread_readFF(&tmp, &value_valid);
      }
    }
    void set_value(const T& value_)
    {
      assert(!has_value());
      value = value_;
      aligned_t tmp = true;
      qthread_writeF(&value_valid, &tmp);
    }
    T& get_value()
    {
      wait();
      return value;
    }
  };
  
  template<>
  class future_state<void> {
    aligned_t value_valid;
  public:
    future_state(): value_valid(false) {}
    bool has_value() const { return value_valid; }
    void wait() const
    {
      if (!has_value()) {
        aligned_t tmp;
        qthread_readFF(&tmp, &value_valid);
      }
    }
    void set_value()
    {
      assert(!has_value());
      aligned_t tmp = true;
      qthread_writeF(&value_valid, &tmp);
    }
    void get_value()
    {
      wait();
    }
  };
  
  
  template<typename T>
  class shared_future {
    shared_ptr<future_state<T>> state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const shared_ptr<future_state<T>>& state): state(state) {}
    friend class promise<T>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<T>&& other);
    shared_future(shared_future&& other): shared_future() { swap(other); }
    shared_future(const shared_future& other) = delete;
    ~shared_future() {}
    shared_future& operator=(shared_future&& other)
    {
      swap(other);
      other.state.reset();
      return *this;
    }
    shared_future& operator=(const shared_future& other) = delete;
    // shared_future<T> share() { return shared_future<T>(*this); }
    // T get() { return state->get_value(); };
    const T& get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<>
  class shared_future<void> {
    shared_ptr<future_state<void>> state;
    void swap(shared_future& other) { std::swap(state, other.state); }
    shared_future(const shared_ptr<future_state<void>>& state): state(state) {}
    friend class promise<void>;
  public:
    shared_future(): state(nullptr) {}
    shared_future(future<void>&& other);
    shared_future(shared_future&& other): shared_future() { swap(other); }
    shared_future(const shared_future& other) = delete;
    ~shared_future() {}
    shared_future& operator=(shared_future&& other)
    {
      swap(other);
      other.state.reset();
      return *this;
    }
    shared_future& operator=(const shared_future& other) = delete;
    // shared_future<void> share() { return shared_future<void>(*this); }
    // void get() { return state->get_value(); };
    void get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<typename T>
  class future {
    shared_ptr<future_state<T>> state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const shared_ptr<future_state<T>>& state): state(state) {}
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
    shared_future<T> share() { return shared_future<T>(*this); }
    T get() { return state->get_value(); };
    // const T& get() const { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<>
  class future<void> {
    shared_ptr<future_state<void>> state;
    void swap(future& other) { std::swap(state, other.state); }
    future(const shared_ptr<future_state<void>>& state): state(state) {}
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
    shared_future<void> share() { return shared_future<void>(move(*this)); }
    void get() { return state->get_value(); };
    bool valid() const { return bool(state); }
    void wait() const { state->wait(); }
  };
  
  template<typename T>
  shared_future<T>::shared_future(future<T>&& other)
  {
    std::swap(state, other.state);
  }
  
  inline shared_future<void>::shared_future(future<void>&& other)
  {
    std::swap(state, other.state);
  }
  
  template<typename T>
  class promise {
    shared_ptr<future_state<T>> state;
  public:
    promise(): state(make_shared<future_state<T>>()) {}
    promise(promise&& other): state() { std::swap(*this, other); }
    promise(const promise& other) = delete;
    ~promise() { assert(!state || state->has_value()); }
    promise& operator=(promise&& other)
    {
      std::swap(*this, other);
      other.state.reset();
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    future<T> get_future() { return future<T>(state); }
    void set_value(const T& value) { state->set_value(value); }
    // void set_value(T&& value) { state->set_value(value); }
  };
  
  template<>
  class promise<void> {
    shared_ptr<future_state<void>> state;
  public:
    promise(): state(make_shared<future_state<void>>()) {}
    promise(promise&& other): state() { std::swap(*this, other); }
    promise(const promise& other) = delete;
    ~promise() { assert(!state || state->has_value()); }
    promise& operator=(promise&& other)
    {
      std::swap(*this, other);
      other.state.reset();
    }
    promise& operator=(const promise& rhs) = delete;
    void swap(promise& other) { std::swap(state, other.state); }
    future<void> get_future() { return future<void>(state); }
    void set_value() { state->set_value(); }
    // void set_value() { state->set_value(); }
  };
  
  template<typename T>
  void swap(promise<T>& lhs, promise<T>& rhs)
  {
    lhs.swap(rhs);
  }
  
}

#define QTHREAD_FUTURE_HH_DONE
#else
#  ifndef QTHREAD_FUTURE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_FUTURE_HH
