#ifndef QTHREAD_FUTURE_HH
#define QTHREAD_FUTURE_HH

#include "qthread_future_fwd.hh"

#include "qthread_thread_fwd.hh"

#include "cxx_tuple.hh"
#include "cxx_utils.hh"

#include <qthread/qthread.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace qthread {

template <typename T>
future<typename std::decay<T>::type> make_ready_future(T &&value);

namespace detail {
template <typename T> struct is_future : std::false_type {};
template <typename T> struct is_future<future<T> > : std::true_type {};
template <typename T> struct is_future<shared_future<T> > : std::true_type {};
}

template <typename T> class future_state {
  syncvar m_ready;
  std::function<T()> m_deferred;
  std::atomic<char> deferred_is_running;
  bool has_exception;
  T value;

public:
  future_state() : has_exception(false) { m_ready.empty(); }
  future_state(const std::function<T()> &deferred) : future_state() {
    m_deferred = deferred;
    deferred_is_running = false;
  }
  ~future_state() { m_ready.fill(); }
  future_state(const future_state &) = delete;
  future_state(future_state &&) = delete;
  future_state &operator=(const future_state &) = delete;
  future_state &operator=(future_state &&) = delete;
  bool is_ready() { return m_ready.status(); }
  void wait() {
    if (m_deferred) {
      if (!bool(deferred_is_running.fetch_or(true))) {
        RPC_ASSERT(!is_ready());
        value = m_deferred();
        m_deferred = {};
        m_ready.fill();
      }
    }
    m_ready.readFF();
  }
  void set_value(const T &value_) {
    RPC_ASSERT(!is_ready() && !(m_deferred && bool(deferred_is_running)));
    value = value_;
    m_ready.fill();
  }
  void set_value(T &&value_) {
    RPC_ASSERT(!is_ready() && !(m_deferred && bool(deferred_is_running)));
    std::swap(value, value_);
    m_ready.fill();
  }
  void set_exception() {
    RPC_ASSERT(!is_ready() && !(m_deferred && bool(deferred_is_running)));
    has_exception = true;
    RPC_ASSERT(0); // TODO
    m_ready.fill();
  }
  T &get_value() {
    wait();
    RPC_ASSERT(!has_exception);
    return value;
  }
};

template <> class future_state<void> {
  syncvar m_ready;
  std::function<void()> m_deferred;
  std::atomic<char> deferred_is_running;
  bool has_exception;

public:
  future_state() : has_exception(false) { m_ready.empty(); }
  future_state(const std::function<void()> &deferred) : future_state() {
    m_deferred = deferred;
    deferred_is_running = false;
  }
  ~future_state() { m_ready.fill(); }
  future_state(const future_state &) = delete;
  future_state(future_state &&) = delete;
  future_state &operator=(const future_state &) = delete;
  future_state &operator=(future_state &&) = delete;
  bool is_ready() { return m_ready.status(); }
  void wait() {
    if (m_deferred) {
      if (!bool(deferred_is_running.fetch_or(true))) {
        RPC_ASSERT(!is_ready());
        m_deferred();
        m_deferred = {};
        m_ready.fill();
      }
    }
    m_ready.readFF();
  }
  void set_value() {
    RPC_ASSERT(!is_ready() && !(m_deferred && bool(deferred_is_running)));
    m_ready.fill();
  }
  void set_exception() {
    RPC_ASSERT(!is_ready() && !(m_deferred && bool(deferred_is_running)));
    has_exception = true;
    RPC_ASSERT(0); // TODO
    m_ready.fill();
  }
  void get_value() {
    wait();
    RPC_ASSERT(!has_exception);
  }
};

template <typename T> class shared_future {
  typedef T value_type;
  std::shared_ptr<future_state<value_type> > state;
  void swap(shared_future &other) { std::swap(state, other.state); }
  shared_future(const std::shared_ptr<future_state<value_type> > &state)
      : state(state) {}
  friend class future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  shared_future() : state(nullptr) {}
  shared_future(future<value_type> &&other);
  shared_future(const shared_future &other) : state(other.state) {}
  shared_future(shared_future &&other) : shared_future() { swap(other); }
  ~shared_future() {}
  shared_future &operator=(const shared_future &other) {
    state = other.state;
    return *this;
  }
  shared_future &operator=(shared_future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  const value_type &get() const { return state->get_value(); };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(const F &func) const
      -> future<typename cxx::invoke_of<F, const shared_future &>::type> {
    RPC_ASSERT(valid());
    // TODO: move func instead of copying it
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, *this));
    } else {
      // TODO: optimize this
      // TODO: create a packaged_task, return its future; store a
      // function that runs the packaged_task
      return async([func](const shared_future &f) {
                     f.wait();
                     return cxx::invoke(func, f);
                   },
                   *this);
    }
  }
  template <typename U = T>
  typename std::enable_if<
      ((std::is_same<U, T>::value) && (detail::is_future<T>::value)),
      shared_future<typename U::value_type> >::type
  unwrap() const {
    RPC_ASSERT(valid());
    // TODO: optimize this
    return then([](const shared_future &f) { return f.get().get(); });
  }
  bool is_ready() const { return state->is_ready(); }
};

template <typename T> class shared_future<T &> {
  typedef T &value_type;
  std::shared_ptr<future_state<T> > state;
  void swap(shared_future &other) { std::swap(state, other.state); }
  shared_future(const std::shared_ptr<future_state<value_type> > &state)
      : state(state) {}
  friend class future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  shared_future() : state(nullptr) {}
  shared_future(future<value_type> &&other);
  shared_future(const shared_future &other) : state(other.state) {}
  shared_future(shared_future &&other) : shared_future() { swap(other); }
  ~shared_future() {}
  shared_future &operator=(const shared_future &other) {
    state = other.state;
    return *this;
  }
  shared_future &operator=(shared_future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  value_type get() const { return state->get_value(); };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(const F &func) const
      -> future<typename cxx::invoke_of<F, const shared_future &>::type> {
    RPC_ASSERT(valid());
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, *this));
    } else {
      // TODO: optimize this
      return async([=](const shared_future &f) {
                     f.wait();
                     return cxx::invoke(func, f);
                   },
                   *this);
    }
  }
  bool is_ready() const { return state->is_ready(); }
};

template <> class shared_future<void> {
  typedef void value_type;
  std::shared_ptr<future_state<value_type> > state;
  void swap(shared_future &other) { std::swap(state, other.state); }
  shared_future(const std::shared_ptr<future_state<value_type> > &state)
      : state(state) {}
  friend class future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  shared_future() : state(nullptr) {}
  shared_future(future<value_type> &&other);
  shared_future(const shared_future &other) : state(other.state) {}
  shared_future(shared_future &&other) : shared_future() { swap(other); }
  ~shared_future() {}
  shared_future &operator=(const shared_future &other) {
    state = other.state;
    return *this;
  }
  shared_future &operator=(shared_future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  value_type get() const { return state->get_value(); };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(const F &func) const
      -> future<typename cxx::invoke_of<F, const shared_future &>::type> {
    RPC_ASSERT(valid());
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, *this));
    } else {
      // TODO: optimize this
      return async([=](const shared_future &f) {
                     f.wait();
                     return cxx::invoke(func, f);
                   },
                   *this);
    }
  }
  bool is_ready() const { return state->is_ready(); }
};

template <typename T> class future {
  typedef T value_type;
  std::shared_ptr<future_state<value_type> > state;
  void swap(future &other) { std::swap(state, other.state); }
  future(const std::shared_ptr<future_state<value_type> > &state)
      : state(state) {}
  friend class shared_future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  future() : state(nullptr) {}
  future(future &&other) : future() { swap(other); }
  future(const future &other) = delete;
  ~future() {}
  future &operator=(future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  future &operator=(const future &other) = delete;
  shared_future<value_type> share() {
    return shared_future<value_type>(std::move(*this));
  }
  value_type get() {
    auto value = state->get_value();
    state = nullptr;
    return value;
  };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(const F &func)
      -> future<typename cxx::invoke_of<F, future &&>::type> {
    RPC_ASSERT(valid());
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, std::move(*this)));
    } else {
      // TODO: optimize this
      return async([func](future &&f) {
                     f.wait();
                     return cxx::invoke(func, std::move(f));
                   },
                   std::move(*this));
    }
  }
  template <typename U = T>
  typename std::enable_if<
      ((std::is_same<U, T>::value) && (detail::is_future<T>::value)),
      future<typename U::value_type> >::type
  unwrap() {
    RPC_ASSERT(valid());
    // TODO: optimize this
    return then([](future &&f) { return std::move(f).get().get(); });
  }
  bool is_ready() const { return state->is_ready(); }
};

template <typename T> class future<T &> {
  typedef T &value_type;
  std::shared_ptr<future_state<value_type> > state;
  void swap(future &other) { std::swap(state, other.state); }
  future(const std::shared_ptr<future_state<T> > &state) : state(state) {}
  friend class shared_future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  future() : state(nullptr) {}
  future(future &&other) : future() { swap(other); }
  future(const future &other) = delete;
  ~future() {}
  future &operator=(future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  future &operator=(const future &other) = delete;
  shared_future<value_type> share() {
    return shared_future<T>(std::move(*this));
  }
  value_type get() { return state->get_value(); };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(const F &func)
      -> future<typename cxx::invoke_of<F, future &&>::type> {
    RPC_ASSERT(valid());
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, std::move(*this)));
    } else {
      // TODO: optimize this
      return async([func](future &&f) {
                     f.wait();
                     return cxx::invoke(func, std::move(f));
                   },
                   std::move(*this));
    }
  }
  bool is_ready() const { return state->is_ready(); }
};

template <> class future<void> {
  typedef void value_type;
  std::shared_ptr<future_state<value_type> > state;
  void swap(future &other) { std::swap(state, other.state); }
  future(const std::shared_ptr<future_state<value_type> > &state)
      : state(state) {}
  friend class shared_future<value_type>;
  friend class promise<value_type>;
  friend class deferred<value_type>;

public:
  future() : state(nullptr) {}
  future(future &&other) : future() { swap(other); }
  future(const future &other) = delete;
  ~future() {}
  future &operator=(future &&other) {
    state = nullptr;
    swap(other);
    return *this;
  }
  future &operator=(const future &other) = delete;
  shared_future<value_type> share() {
    return shared_future<value_type>(std::move(*this));
  }
  value_type get() { return state->get_value(); };
  bool valid() const { return bool(state); }
  void wait() const { state->wait(); }
  template <typename F>
  auto then(F &&func) -> future<typename cxx::invoke_of<F, future &&>::type> {
    RPC_ASSERT(valid());
    if (is_ready()) {
      return make_ready_future(cxx::invoke(func, std::move(*this)));
    } else {
      // TODO: optimize this
      return async([func](future &&f) {
                     f.wait();
                     return cxx::invoke(func, std::move(f));
                   },
                   std::move(*this));
    }
  }
  bool is_ready() const { return state->is_ready(); }
};

template <typename T>
shared_future<T>::shared_future(future<T> &&other)
    : state(nullptr) {
  std::swap(state, other.state);
}

template <typename T> class promise {
  std::shared_ptr<future_state<T> > state;

public:
  promise() : state(std::make_shared<future_state<T> >()) {}
  promise(promise &&other) : state(nullptr) { swap(other); }
  promise(const promise &other) = delete;
  ~promise() {
    if (!state)
      return;
    if (state.unique())
      return;
    if (!state->is_ready())
      set_exception(); // cannot handle this yet
  }
  promise &operator=(promise &&other) {
    state = nullptr;
    std::swap(*this, other);
    return *this;
  }
  promise &operator=(const promise &rhs) = delete;
  void swap(promise &other) { std::swap(state, other.state); }
  // TODO: allow only one call to get_future
  future<T> get_future() {
    RPC_ASSERT(state != nullptr);
    return future<T>(state);
  }
  void set_value(const T &value) { state->set_value(value); }
  void set_value(T &&value) { state->set_value(std::forward<T>(value)); }
  void set_exception() { state->set_exception(); }
};

template <typename T> class promise<T &> {
  std::shared_ptr<future_state<T> > state;

public:
  promise() : state(std::make_shared<future_state<T> >()) {}
  promise(promise &&other) : state(nullptr) { swap(other); }
  promise(const promise &other) = delete;
  ~promise() {
    if (state && !state->is_ready())
      set_exception();
  }
  promise &operator=(promise &&other) {
    state = nullptr;
    std::swap(*this, other);
    return *this;
  }
  promise &operator=(const promise &rhs) = delete;
  void swap(promise &other) { std::swap(state, other.state); }
  // TODO: allow only one call to get_future
  future<T &> get_future() {
    RPC_ASSERT(state);
    return future<T &>(state);
  }
  void set_value(T &value) { state->set_value(value); }
  void set_exception() { state->set_exception(); }
};

template <> class promise<void> {
  std::shared_ptr<future_state<void> > state;

public:
  promise() : state(std::make_shared<future_state<void> >()) {}
  promise(promise &&other) : state(nullptr) { swap(other); }
  promise(const promise &other) = delete;
  ~promise() {
    if (!state)
      return;
    if (state.unique())
      return;
    if (!state->is_ready())
      set_exception(); // cannot handle this yet
  }
  promise &operator=(promise &&other) {
    state = nullptr;
    std::swap(*this, other);
    return *this;
  }
  promise &operator=(const promise &rhs) = delete;
  void swap(promise &other) { std::swap(state, other.state); }
  future<void> get_future() { return future<void>(state); }
  void set_value() { state->set_value(); }
  void set_exception() { state->set_exception(); }
};

template <typename T> void swap(promise<T> &lhs, promise<T> &rhs) {
  lhs.swap(rhs);
}

template <typename T> class deferred {
  std::shared_ptr<future_state<T> > state;

public:
  deferred() {}
  deferred(const std::function<T()> func)
      : state(std::make_shared<future_state<T> >(func)) {}
  deferred(deferred &&other) : deferred() { swap(other); }
  deferred(const deferred &other) = delete;
  deferred &operator=(deferred &&other) {
    std::swap(*this, other);
    return *this;
  }
  deferred &operator=(const deferred &rhs) = delete;
  void swap(deferred &other) { std::swap(state, other.state); }
  // TODO: allow only one call to get_future
  future<T> get_future() {
    RPC_ASSERT(state != nullptr);
    return future<T>(state);
  }
};

template <typename T> void swap(deferred<T> &lhs, deferred<T> &rhs) {
  lhs.swap(rhs);
}

template <typename T>
future<typename std::decay<T>::type> make_ready_future(T &&value) {
  promise<typename std::decay<T>::type> p;
  p.set_value(std::forward<T>(value));
  return p.get_future();
}

inline future<void> make_ready_future() {
  promise<void> p;
  p.set_value();
  return p.get_future();
}

template <typename T> inline bool future_is_ready(const shared_future<T> &f) {
  return f.is_ready();
}

template <typename T> inline bool future_is_ready(const future<T> &f) {
  return f.is_ready();
}

template <typename T, typename F>
inline auto future_then(const shared_future<T> &f, F &&func)
    -> future<typename cxx::invoke_of<F, const shared_future<T> &>::type> {
  return f.then(func);
}

template <typename T, typename F>
inline auto future_then(future<T> &&f, F &&func)
    -> future<typename cxx::invoke_of<F, future<T> &&>::type> {
  return std::move(f).then(func);
}
}

#define QTHREAD_FUTURE_HH_DONE
#else
#ifndef QTHREAD_FUTURE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // QTHREAD_FUTURE_HH
