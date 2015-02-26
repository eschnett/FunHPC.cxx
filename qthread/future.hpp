#ifndef QTHREAD_FUTURE_HPP
#define QTHREAD_FUTURE_HPP

#include <cxx/apply.hpp>
#include <cxx/invoke.hpp>
#include <cxx/task.hpp>

#include <qthread/qthread.hpp>

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <tuple>

namespace qthread {

// shared_state ////////////////////////////////////////////////////////////////

namespace detail {
template <typename T> class shared_state {
  // id_t<T> is T, but if used in a function template, the compiler
  // cannot deduce T from it
  template <typename U> struct id { typedef U type; };
  template <typename U> using id_t = typename id<U>::type;

  mutable syncvar is_ready;

  // if present, needs to be called once when waiting
  std::atomic<bool> has_trigger;
  cxx::task<T> trigger;

  // void is stored as empty tuple, references are stored as pointers
  std::conditional_t<std::is_void<T>::value, std::tuple<>,
                     std::conditional_t<std::is_reference<T>::value,
                                        std::remove_reference_t<T> *, T>> value;

  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value> * = nullptr>
  void run_trigger() {
    set_value(trigger());
  }
  template <typename U = T,
            std::enable_if_t<std::is_void<U>::value> * = nullptr>
  void run_trigger() {
    trigger();
    set_value();
  }

public:
  shared_state() : has_trigger(false) { is_ready.empty(); }
  ~shared_state() { is_ready.fill(); }

  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value &&
                             !std::is_reference<U>::value> * = nullptr>
  shared_state(id_t<U> &&value)
      : has_trigger(false), value(std::move(value)) {}
  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value &&
                             !std::is_reference<U>::value> * = nullptr>
  shared_state(const id_t<U> &value)
      : has_trigger(false), value(value) {}
  template <typename U = T,
            std::enable_if_t<std::is_reference<U>::value> * = nullptr>
  shared_state(id_t<U> &value)
      : has_trigger(false), value(&value) {}
  template <typename U = T,
            std::enable_if_t<std::is_void<U>::value> * = nullptr>
  shared_state(std::tuple<>)
      : has_trigger(false), value(std::tuple<>()) {}

  shared_state(const cxx::task<T> &trigger) : shared_state() {
    has_trigger = true;
    this->trigger = trigger;
  }
  shared_state(cxx::task<T> &&trigger) : shared_state() {
    has_trigger = true;
    this->trigger = std::move(trigger);
  }

  shared_state(const shared_state &) = delete;
  shared_state(shared_state &&) = delete;
  shared_state &operator=(const shared_state &) = delete;
  shared_state &operator=(shared_state &&) = delete;

  bool ready() const noexcept { return is_ready.status(); }

  void wait() {
    if (bool(has_trigger) && has_trigger.exchange(false)) {
      run_trigger();
      trigger = {};
    }
    is_ready.readFF();
  }

  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value &&
                             !std::is_reference<U>::value> * = nullptr>
  void set_value(id_t<U> &&value_) {
    assert(!ready());
    value = std::move(value_); /*TODO: memory order */
    is_ready.fill();
  }
  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value &&
                             !std::is_reference<U>::value> * = nullptr>
  void set_value(const id_t<U> &value_) {
    static_assert(!std::is_reference<T>::value, "");
    assert(!ready());
    value = value_; /*TODO: memory order */
    is_ready.fill();
  }
  template <typename U = T,
            std::enable_if_t<std::is_reference<U>::value> * = nullptr>
  void set_value(id_t<U> &value_) {
    assert(!ready());
    value = &value_; /*TODO: memory order */
    is_ready.fill();
  }
  template <typename U = T,
            std::enable_if_t<std::is_void<U>::value> * = nullptr>
  void set_value() {
    assert(!ready());
    is_ready.fill();
  }

  void set_exception() { throw("not implemented"); }

  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             !std::is_void<T>::value> * = nullptr>
  const U &get() const {
    wait();
    return value;
  }
  template <
      typename U = T,
      std::enable_if_t<std::is_same<U, T>::value && !std::is_void<T>::value &&
                       !std::is_reference<T>::value> * = nullptr>
  U &get() {
    wait();
    return value;
  }
  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             std::is_reference<T>::value> * = nullptr>
  U get() {
    wait();
    return *value;
  }
  template <
      typename U = T,
      std::enable_if_t<std::is_same<U, T>::value && !std::is_void<T>::value &&
                       !std::is_reference<T>::value> * = nullptr>
  U move() {
    wait();
    return std::move(value);
    // Note: The value is now not available any more
  }
};
}

// future //////////////////////////////////////////////////////////////////////

template <typename T> class future;
template <typename T> class shared_future;
template <typename T> class promise;

namespace detail {
template <typename T> struct is_future : std::false_type {};
template <typename T> struct is_future<future<T>> : std::true_type {};

template <typename T> struct is_shared_future : std::false_type {};
template <typename T>
struct is_shared_future<shared_future<T>> : std::true_type {};
}

enum class launch : unsigned;

namespace detail {
template <typename T>
future<T> make_future_with_shared_state(
    std::shared_ptr<detail::shared_state<T>> &&shared_state);
}

template <typename T> class future {
  template <typename U> friend class future;
  template <typename U> friend class shared_future;
  template <typename U> friend class promise;

  friend future<T> detail::make_future_with_shared_state<T>(
      std::shared_ptr<detail::shared_state<T>> &&shared_state);

  typedef T element_type;

  std::shared_ptr<detail::shared_state<T>> shared_state;

  future(const std::shared_ptr<detail::shared_state<T>> &shared_state)
      : shared_state(shared_state) {}
  future(std::shared_ptr<detail::shared_state<T>> &&shared_state)
      : shared_state(std::move(shared_state)) {}

public:
  future() noexcept : shared_state() {}
  future(future &&other) noexcept : future() { swap(other); }
  future(const future &other) = delete;

  future(future<future<T>> &&other) noexcept; // unwrap

  future &operator=(future &&other) noexcept {
    shared_state.reset();
    swap(other);
    return *this;
  }
  future &operator=(const future &other) = delete;

  void swap(future &other) noexcept {
    using std::swap;
    swap(shared_state, other.shared_state);
  }

  shared_future<T> share() {
    assert(valid());
    return shared_future<T>(std::move(*this));
  }

  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value &&
                             !std::is_reference<U>::value> * = nullptr>
  U get() {
    assert(valid());
    auto res(shared_state->move());
    shared_state.reset();
    return res;
  }
  template <typename U = T,
            std::enable_if_t<std::is_reference<U>::value> * = nullptr>
  U &get() {
    assert(valid());
    decltype(auto) res(shared_state->get());
    shared_state.reset();
    return res;
  }
  template <typename U = T,
            std::enable_if_t<std::is_void<U>::value> * = nullptr>
  void get() {
    assert(valid());
    shared_state->wait();
    shared_state.reset();
  }

  bool valid() const noexcept { return bool(shared_state); }

  bool ready() const {
    assert(valid());
    return shared_state->ready();
  }

  void wait() const {
    assert(valid());
    shared_state->wait();
  }

  template <typename F, typename R = cxx::invoke_of_t<std::decay_t<F>, future>>
  future<R> then(launch policy, F &&cont);
  template <typename F, typename R = cxx::invoke_of_t<std::decay_t<F>, future>>
  future<R> then(F &&cont);

  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             detail::is_future<U>::value> * = nullptr>
  future<typename U::element_type> unwrap();
  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             detail::is_shared_future<U>::value> * = nullptr>
  future<typename U::element_type> unwrap();
};
template <typename T> void swap(future<T> &lhs, future<T> &rhs) noexcept {
  lhs.swap(rhs);
}

namespace detail {
template <typename T>
future<T> make_future_with_shared_state(
    std::shared_ptr<detail::shared_state<T>> &&shared_state) {
  return future<T>(std::move(shared_state));
}
}

// make_ready_future ///////////////////////////////////////////////////////////

template <typename T> future<std::decay_t<T>> make_ready_future(T &&value) {
  return detail::make_future_with_shared_state(
      std::make_shared<detail::shared_state<std::decay_t<T>>>(
          std::forward<T>(value)));
}
inline future<void> make_ready_future() {
  return detail::make_future_with_shared_state(
      std::make_shared<detail::shared_state<void>>(std::tuple<>()));
}

// shared_future ///////////////////////////////////////////////////////////////

template <typename T> class shared_future {
  template <typename U> friend class future;
  template <typename U> friend class shared_future;

  std::shared_ptr<detail::shared_state<T>> shared_state;

  typedef T element_type;

public:
  shared_future() noexcept : shared_state() {}
  shared_future(const shared_future &other)
      : shared_state(other.shared_state) {}
  shared_future(future<T> &&other)
      : shared_state(std::move(other.shared_state)) {}
  shared_future(shared_future &&other) noexcept : shared_future() {
    swap(other);
  }

  shared_future(const shared_future<shared_future> &other); // unwrap
  shared_future(shared_future<shared_future> &&other);      // unwrap

  shared_future &operator=(const shared_future &other) {
    shared_state = other.shared_state;
    return *this;
  }
  shared_future &operator=(shared_future &&other) noexcept {
    shared_state = std::move(other.shared_state);
    return *this;
  }
  shared_future &operator=(future<T> &&other) {
    shared_state = std::move(other.shared_state);
    return *this;
  }

  void swap(shared_future &other) noexcept {
    using std::swap;
    swap(shared_state, other.shared_state);
  }

  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value> * = nullptr>
  const U &get() const {
    assert(valid());
    return shared_state->get();
  }
  template <typename U = T,
            std::enable_if_t<!std::is_void<U>::value> * = nullptr>
  U &get() {
    assert(valid());
    return shared_state->get();
  }
  template <typename U = T,
            std::enable_if_t<std::is_void<U>::value> * = nullptr>
  void get() const {
    assert(valid());
    shared_state->wait();
  }

  bool valid() const noexcept { return bool(shared_state); }

  bool ready() const noexcept {
    assert(valid());
    return shared_state->ready();
  }

  void wait() const {
    assert(valid());
    shared_state->wait();
  }

  template <typename F, typename R = cxx::invoke_of_t<F &&, shared_future>>
  future<R> then(launch policy, F &&cont);
  template <typename F, typename R = cxx::invoke_of_t<F &&, shared_future>>
  future<R> then(F &&cont);

  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             detail::is_future<U>::value> * = nullptr>
  future<typename U::element_type> unwrap() const;
  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             detail::is_shared_future<U>::value> * = nullptr>
  future<typename U::element_type> unwrap() const;
};
template <typename T>
void swap(shared_future<T> &lhs, shared_future<T> &rhs) noexcept {
  lhs.swap(rhs);
}

// promise /////////////////////////////////////////////////////////////////////

template <typename T> class promise {
  std::shared_ptr<detail::shared_state<T>> shared_state;

public:
  // promise() : shared_state(std::make_shared<detail::shared_state<T>>()) {}
  promise() {}
  promise(promise &&other) noexcept { swap(other); }
  promise(const promise &other) = delete;

  ~promise() {
    if (shared_state && !shared_state.unique() && !shared_state->ready())
      shared_state->set_exception();
  }

  promise &operator=(promise &&other) {
    shared_state.reset();
    swap(other);
    return *this;
  }
  promise &operator=(const promise &other) = delete;

  void swap(promise &other) noexcept {
    using std::swap;
    swap(shared_state, other.shared_state);
  }

  future<T> get_future() {
    if (!shared_state)
      shared_state = std::make_shared<detail::shared_state<T>>();
    return future<T>(shared_state);
  }

  template <
      typename U = T,
      std::enable_if_t<std::is_same<U, T>::value && !std::is_void<T>::value &&
                       !std::is_reference<T>::value> * = nullptr>
  void set_value(const U &value) {
    if (shared_state)
      shared_state->set_value(value);
    else
      shared_state = std::make_shared<detail::shared_state<T>>(value);
  }
  template <
      typename U = T,
      std::enable_if_t<std::is_same<U, T>::value && !std::is_void<T>::value &&
                       !std::is_reference<T>::value> * = nullptr>
  void set_value(U &&value) {
    if (shared_state)
      shared_state->set_value(std::move(value));
    else
      shared_state =
          std::make_shared<detail::shared_state<T>>(std::move(value));
  }
  template <typename U = T,
            std::enable_if_t<std::is_same<T, U &>::value &&
                             std::is_reference<T>::value> * = nullptr>
  void set_value(U &value) {
    if (shared_state)
      shared_state->set_value(value);
    else
      shared_state = std::make_shared<detail::shared_state<T>>(value);
  }
  template <typename U = T,
            std::enable_if_t<std::is_same<U, T>::value &&
                             std::is_void<T>::value> * = nullptr>
  void set_value() {
    if (shared_state)
      shared_state->set_value();
    else
      shared_state = std::make_shared<detail::shared_state<T>>(std::tuple<>());
  }
};
template <typename T> void swap(promise<T> &lhs, promise<T> &rhs) noexcept {
  lhs.swap(rhs);
}

// packaged_task ///////////////////////////////////////////////////////////////

template <typename> class packaged_task;
template <typename R, typename... Args> class packaged_task<R(Args...)> {
  std::function<R(Args...)> task;
  promise<R> result;

public:
  packaged_task() noexcept {}
  template <
      class F,
      std::enable_if_t<
          !std::is_same<std::decay_t<F>, packaged_task<R(Args...)>>::value> * =
          nullptr>
  explicit packaged_task(F &&f)
      : task(std::forward<F>(f)) {}
  packaged_task(const packaged_task &) = delete;
  packaged_task(packaged_task &&other) noexcept : packaged_task() {
    swap(other);
  }

  packaged_task &operator=(packaged_task &&other) noexcept {
    task = {};
    result = {};
    swap(other);
    return *this;
  }
  packaged_task &operator=(const packaged_task &) = delete;

  bool valid() const noexcept { return bool(task); }

  void swap(packaged_task &other) noexcept {
    using std::swap;
    swap(task, other.task);
    swap(result, other.result);
  }

  future<R> get_future() {
    assert(bool(task));
    return result.get_future();
  }

  template <typename U = R,
            typename std::enable_if<std::is_same<U, R>::value &&
                                    !std::is_void<R>::value>::type * = nullptr>
  void operator()(Args... args) {
    assert(bool(task));
    result.set_value(task(std::forward<Args>(args)...));
  }
  template <typename U = R,
            typename std::enable_if<std::is_same<U, R>::value &&
                                    std::is_void<R>::value>::type * = nullptr>
  void operator()(Args... args) {
    assert(bool(task));
    task(std::forward<Args>(args)...);
    result.set_value();
  }

  void reset() {
    assert(valid());
    result = {};
  }
};
template <typename R, typename... Args>
void swap(packaged_task<R(Args...)> &lhs,
          packaged_task<R(Args...)> &rhs) noexcept {
  lhs.swap(rhs);
}

// async_thread ////////////////////////////////////////////////////////////////

namespace detail {
template <typename R> class async_thread {

  future<R> result;

  struct thread_args_t {
    cxx::task<R> task;
    promise<R> result;
    template <typename U = R,
              std::enable_if_t<!std::is_void<U>::value> * = nullptr>
    void run() {
      result.set_value(task());
    }
    template <typename U = R,
              std::enable_if_t<std::is_void<U>::value> * = nullptr>
    void run() {
      task();
      result.set_value();
    }
  };
  static aligned_t run_thread(void *args_) {
    auto thread_args = (thread_args_t *)args_;
    thread_args->run();
    delete thread_args;
    return 1;
  }

public:
  async_thread() noexcept {}
  async_thread(async_thread &&other) noexcept : async_thread() { swap(other); }

  template <
      class F, class... Args,
      std::enable_if_t<!std::is_same<std::decay_t<F>, async_thread>::value> * =
          nullptr>
  explicit async_thread(F &&f, Args &&... args) {
    auto thread_args = new thread_args_t;
    thread_args->task =
        cxx::task<R>(std::forward<F>(f), std::forward<Args>(args)...);
    result = thread_args->result.get_future();
    auto ierr = qthread_fork_syncvar(run_thread, thread_args, nullptr);
    assert(!ierr);
  }

  async_thread(const async_thread &) = delete;

  ~async_thread() { assert(!joinable()); }

  async_thread &operator=(async_thread &&other) {
    assert(!joinable());
    result = future<R>();
    swap(other);
    return *this;
  }
  async_thread &operator=(const async_thread &other) = delete;

  void swap(async_thread &other) noexcept {
    using std::swap;
    swap(result, other.result);
  }

  bool joinable() const noexcept { return result.valid(); }

  void join() {
    assert(result.valid());
    result.wait();
    result = {};
  }

  void detach() { result = {}; }

  // non-standard
  future<R> detach_get_future() { return std::move(result); }
};
template <typename R>
void swap(async_thread<R> &lhs, async_thread<R> &rhs) noexcept {
  lhs.swap(rhs);
}
}

// launch //////////////////////////////////////////////////////////////////////

enum class launch : unsigned {
  async = 1,
  deferred = 2,
  sync = 4,
  detached = 8,
};

inline constexpr launch operator~(launch a) {
  return static_cast<launch>(~static_cast<unsigned>(a));
}

inline constexpr launch operator&(launch a, launch b) {
  return static_cast<launch>(static_cast<unsigned>(a) &
                             static_cast<unsigned>(b));
}
inline constexpr launch operator|(launch a, launch b) {
  return static_cast<launch>(static_cast<unsigned>(a) |
                             static_cast<unsigned>(b));
}
inline constexpr launch operator^(launch a, launch b) {
  return static_cast<launch>(static_cast<unsigned>(a) ^
                             static_cast<unsigned>(b));
}

inline launch &
operator&=(launch &a, launch b) {
  return a = a & b;
}
inline launch &operator|=(launch &a, launch b) { return a = a | b; }
inline launch &operator^=(launch &a, launch b) { return a = a ^ b; }

namespace detail {
// Convert bitmask to a specific policy
constexpr launch decode_policy(launch policy) {
  if ((policy | launch::async) == launch::async)
    return launch::async;
  if ((policy | launch::deferred) == launch::deferred)
    return launch::deferred;
  if ((policy | launch::sync) == launch::sync)
    return launch::sync;
  if ((policy | launch::detached) == launch::detached)
    return launch::detached;
  return launch::async;
}
}

// async ///////////////////////////////////////////////////////////////////////

namespace detail {
template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
std::enable_if_t<!std::is_void<R>::value, future<R>>
async_make_ready_future(F &&f, Args &&... args) {
  return make_ready_future(
      cxx::invoke(cxx::decay_copy(std::forward<F>(f)),
                  cxx::decay_copy(std::forward<Args>(args))...));
}
template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
std::enable_if_t<std::is_void<R>::value, future<R>>
async_make_ready_future(F &&f, Args &&... args) {
  cxx::invoke(cxx::decay_copy(std::forward<F>(f)),
              cxx::decay_copy(std::forward<Args>(args))...);
  return make_ready_future();
}
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
future<R> async(launch policy, F &&f, Args &&... args) {
  switch (detail::decode_policy(policy)) {
  case launch::async:
    return detail::async_thread<R>(std::forward<F>(f),
                                   std::forward<Args>(args)...)
        .detach_get_future();
  case launch::deferred:
    return detail::make_future_with_shared_state(
        std::make_shared<detail::shared_state<R>>(
            cxx::task<R>(std::forward<F>(f), std::forward<Args>(args)...)));
  case launch::sync:
    return detail::async_make_ready_future(std::forward<F>(f),
                                           std::forward<Args>(args)...);
  case launch::detached:
    detail::async_thread<R>(std::forward<F>(f), std::forward<Args>(args)...)
        .detach();
    return future<R>();
  }
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
future<R> async(F &&f, Args &&... args) {
  return async(launch::async | launch::deferred, std::forward<F>(f),
               std::forward<Args>(args)...);
}

// more future /////////////////////////////////////////////////////////////////

template <typename T>
future<T>::future(future<future<T>> &&other) noexcept
    : future(async([other = std::move(other)]() {
        return other.get().get();
      })) {}

template <typename T>
template <typename F, typename R>
future<R> future<T>::then(launch policy, F &&cont) {
  if (!valid())
    return future<R>();
  // TODO: if *this is deferred, wait immediately
  return async(policy, [ftr = std::move(*this)](auto &&cont) mutable {
                         ftr.wait();
                         return cxx::invoke(std::move(cont), std::move(ftr));
                       },
               std::forward<F>(cont));
}
template <typename T>
template <typename F, typename R>
future<R> future<T>::then(F &&cont) {
  // TODO: same as policy of *this
  return then(launch::async | launch::deferred, std::forward<F>(cont));
}

template <typename T>
template <typename U, std::enable_if_t<std::is_same<U, T>::value &&
                                       detail::is_future<U>::value> *>
future<typename U::element_type> future<T>::unwrap() {
  return async([ftr = std::move(*this)]() mutable { return ftr.get().get(); });
}
template <typename T>
template <typename U, std::enable_if_t<std::is_same<U, T>::value &&
                                       detail::is_shared_future<U>::value> *>
future<typename U::element_type> future<T>::unwrap() {
  return async([ftr = std::move(*this)]() mutable { return ftr.get().get(); });
}

// more shared_future //////////////////////////////////////////////////////////

template <typename T>
template <typename F, typename R>
future<R> shared_future<T>::then(launch policy, F &&cont) {
  if (!valid())
    return future<R>();
  // TODO: if *this is deferred, wait immediately
  return async(policy, [ftr = *this](auto &&cont) {
                         ftr.wait();
                         return cxx::invoke(std::move(cont), std::move(ftr));
                       },
               std::forward<F>(cont));
}
template <typename T>
template <typename F, typename R>
future<R> shared_future<T>::then(F &&cont) {
  // TODO: same as policy of *this
  return then(launch::async | launch::deferred, std::forward<F>(cont));
}

template <typename T>
template <typename U, std::enable_if_t<std::is_same<U, T>::value &&
                                       detail::is_future<U>::value> *>
future<typename U::element_type> shared_future<T>::unwrap() const {
  return async([ftr = *this]() mutable { return ftr.get().get(); });
}
template <typename T>
template <typename U, std::enable_if_t<std::is_same<U, T>::value &&
                                       detail::is_shared_future<U>::value> *>
future<typename U::element_type> shared_future<T>::unwrap() const {
  return async([ftr = *this]() { return ftr.get().get(); });
}
}

#define QTHREAD_FUTURE_HPP_DONE
#endif // #ifndef QTHREAD_FUTURE_HPP
#ifndef QTHREAD_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
