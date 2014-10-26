#ifndef RPC_CALL_FWD_HH
#define RPC_CALL_FWD_HH

#include "rpc_global_ptr_fwd.hh"

namespace rpc {

// Remote call policies

enum class rlaunch : int { async = 1, deferred = 2, sync = 4, detached = 8 };

inline constexpr rlaunch operator~(rlaunch a) {
  return static_cast<rlaunch>(~static_cast<int>(a));
}

inline constexpr rlaunch operator&(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<int>(a) & static_cast<int>(b));
}
inline constexpr rlaunch operator|(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<int>(a) | static_cast<int>(b));
}
inline constexpr rlaunch operator^(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<int>(a) ^ static_cast<int>(b));
}

inline rlaunch &
operator&=(rlaunch &a, rlaunch b) {
  return a = a & b;
}
inline rlaunch &operator|=(rlaunch &a, rlaunch b) { return a = a | b; }
inline rlaunch &operator^=(rlaunch &a, rlaunch b) { return a = a ^ b; }

////////////////////////////////////////////////////////////////////////////////

// Base class for all callable RPC objects
struct callable_base {
  virtual ~callable_base() {}
  virtual void execute() = 0;
};

// TODO: combine evaluate and finish classes. instead of a promise
// to fill, add a generic continuation to every action. drop the
// _evaluate suffix.

template <typename F, typename R> struct action_finish : public callable_base {
  global_ptr<promise<R> > p;
  R res;
  action_finish() {} // only for cereal::serialize
  action_finish(const global_ptr<promise<R> > &p, const R &res)
      : p(p), res(res) {}
  virtual void execute() {
    p->set_value(std::move(res));
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, res); }
};
template <typename F> struct action_finish<F, void> : public callable_base {
  global_ptr<promise<void> > p;
  action_finish() {} // only for cereal::serialize
  action_finish(const global_ptr<promise<void> > &p) : p(p) {}
  virtual void execute() {
    p->set_value();
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p); }
};

template <typename F, typename R, typename... As>
struct action_evaluate : public callable_base {
  global_ptr<promise<R> > p;
  std::tuple<typename std::decay<As>::type...> args;
  action_evaluate() {} // only for cereal::serialize
  action_evaluate(const global_ptr<promise<R> > &p, const As &... args)
      : p(p), args(args...) {}
  virtual void execute() {
    R res = cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    server->call(p.get_proc(),
                 std::make_shared<typename F::finish>(p, std::move(res)));
  }
  void execute_locally() {
    R res = cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    p->set_value(std::move(res));
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, args); }
};
template <typename F, typename... As>
struct action_evaluate<F, void, As...> : public callable_base {
  global_ptr<promise<void> > p;
  std::tuple<typename std::decay<As>::type...> args;
  action_evaluate() {} // only for cereal::serialize
  action_evaluate(const global_ptr<promise<void> > &p, const As &... args)
      : p(p), args(args...) {}
  // TODO: Allow moving arguments via &&?
  virtual void execute() {
    cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    server->call(p.get_proc(), std::make_shared<typename F::finish>(p));
  }
  void execute_locally() {
    cxx::tuple_apply(F(), std::move(args));
    if (!p)
      return;
    p->set_value();
    delete p.get();
    p = nullptr;
  }

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(p, args); }
};

// Base type for all actions
template <typename F> struct action_base {};

// // A base types for all actions with a particular signature
// template <typename R, typename... As> struct action {
//   R operator()(As... as) const = 0;
// };

template <typename T, T F> struct wrap {
  typedef T type;
  static constexpr T value = F;
};

////////////////////////////////////////////////////////////////////////////////

// Template for an action
template <typename F, typename W, typename R, typename... As>
struct action_impl_t : public action_base<F> {
  // Note: The argument types As are defined by the function that is
  // wrapped to form this action. As thus cannot adapt for perfect
  // forwarding.
  R operator()(const As &... args) const { return W::value(args...); }
  typedef action_evaluate<F, R, As...> evaluate;
  typedef action_finish<F, R> finish;
};

// Get the action implementation (with its template arguments) for a
// function wrapper
template <typename F, typename W, typename R, typename... As>
action_impl_t<F, W, R, As...> get_action_impl_t(R(As...));

// TODO: don't expect a wrapper, expect the function instead
// TODO: determine the function's type automatically
template <typename F, typename W>
using action_impl = decltype(get_action_impl_t<F, W>(W::value));

// TODO: Implement actions as function pointers (values) instead of types --
// this seems to work

// Example action definition for a given function "f":
// struct f_action:
//   public rpc::action_impl<f_action, rpc::wrap<decltype(&f), &f> >
// {
// };
// RPC_CLASS_EXPORT(f_action::evaluate);
// RPC_CLASS_EXPORT(f_action::finish);

// Call an action on a given destination

// Whether a type is an action
template <typename T> using is_action = std::is_base_of<action_base<T>, T>;

////////////////////////////////////////////////////////////////////////////////

template <typename T> class global_shared_ptr;
template <typename T> class client;

// Whether a type is a global type, and thus provides get_proc()
template <typename T> struct is_global_helper : std::false_type {};
template <typename T>
struct is_global_helper<global_ptr<T> > : std::true_type {};
template <typename T>
struct is_global_helper<global_shared_ptr<T> > : std::true_type {};
template <typename T> struct is_global_helper<client<T> > : std::true_type {};
template <typename T>
struct is_global : is_global_helper<typename std::remove_cv<
                       typename std::remove_reference<T>::type>::type> {};

// Template for a member action
template <typename F, typename W, typename R, typename T, typename... As>
struct member_action_impl_t : public action_base<F> {
  R operator()(const client<T> &obj, const As &... args) const {
    return (*obj.*W::value)(args...);
  }
  typedef action_evaluate<F, R, client<T>, As...> evaluate;
  typedef action_finish<F, R> finish;
};

// Get the member action implementation (with its template
// arguments) for a member function wrapper
// TOOD: Use invoke_of, and remove the need for get_member_action and
// get_const_member_action
template <typename F, typename W, typename R, typename T, typename... As>
member_action_impl_t<F, W, R, T, As...>
get_member_action_impl_t(R (T::*)(As...));
template <typename F, typename W, typename R, typename T, typename... As>
member_action_impl_t<F, W, R, T, As...>
get_const_member_action_impl_t(R (T::*)(As...) const);

// TODO: don't expect a wrapper, expect the function instead
// TODO: determine the function's type automatically
template <typename F, typename W>
using member_action_impl = decltype(get_member_action_impl_t<F, W>(W::value));
template <typename F, typename W>
using const_member_action_impl =
    decltype(get_const_member_action_impl_t<F, W>(W::value));
}

#define RPC_CALL_FWD_HH_DONE
#else
#ifndef RPC_CALL_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CALL_FWD_HH
