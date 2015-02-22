#ifndef FUNHPC_ASYNC_HPP
#define FUNHPC_ASYNC_HPP

#include <cxx/invoke.hpp>
#include <funhpc/rexec.hpp>
#include <funhpc/rptr.hpp>
#include <qthread/future.hpp>

#include <type_traits>

namespace funhpc {

// rlaunch /////////////////////////////////////////////////////////////////////

enum class rlaunch : unsigned {
  async = static_cast<unsigned>(qthread::launch::async),
  deferred = static_cast<unsigned>(qthread::launch::deferred),
  sync = static_cast<unsigned>(qthread::launch::sync),
  detached = static_cast<unsigned>(qthread::launch::detached),
};

inline constexpr rlaunch operator~(rlaunch a) {
  return static_cast<rlaunch>(~static_cast<unsigned>(a));
}

inline constexpr rlaunch operator&(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) &
                              static_cast<unsigned>(b));
}
inline constexpr rlaunch operator|(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) |
                              static_cast<unsigned>(b));
}
inline constexpr rlaunch operator^(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) ^
                              static_cast<unsigned>(b));
}

inline rlaunch &
operator&=(rlaunch &a, rlaunch b) {
  return a = a & b;
}
inline rlaunch &operator|=(rlaunch &a, rlaunch b) { return a = a | b; }
inline rlaunch &operator^=(rlaunch &a, rlaunch b) { return a = a ^ b; }

namespace detail {
// Convert bitmask to a specific policy
constexpr rlaunch decode_policy(rlaunch policy) {
  if ((policy | rlaunch::async) == rlaunch::async)
    return rlaunch::async;
  if ((policy | rlaunch::deferred) == rlaunch::deferred)
    return rlaunch::deferred;
  if ((policy | rlaunch::sync) == rlaunch::sync)
    return rlaunch::sync;
  if ((policy | rlaunch::detached) == rlaunch::detached)
    return rlaunch::detached;
  return rlaunch::async;
}

// Convert policy to a local policy
constexpr qthread::launch local_policy(rlaunch policy) {
  return static_cast<qthread::launch>(policy);
}
}

// async ///////////////////////////////////////////////////////////////////////

namespace detail {
template <typename R>
void set_result(rptr<qthread::promise<R>> rpres, R &&res) {
  static_assert(!std::is_void<R>::value, "");
  auto pres = rpres.get_ptr();
  pres->set_value(std::move(res));
  delete pres;
}
template <typename R> void set_result_void(rptr<qthread::promise<R>> rpres) {
  static_assert(std::is_void<R>::value, "");
  auto pres = rpres.get_ptr();
  pres->set_value();
  delete pres;
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<F &&, Args &&...>,
          std::enable_if_t<!std::is_void<R>::value> * = nullptr>
void continued(rptr<qthread::promise<R>> rpres, F &&f, Args &&... args) {
  rexec(rpres.get_proc(), set_result<R>, rpres,
        cxx::invoke(std::forward<F>(f), std::forward<Args>(args)...));
}
template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<F &&, Args &&...>,
          std::enable_if_t<std::is_void<R>::value> * = nullptr>
void continued(rptr<qthread::promise<R>> rpres, F &&f, Args &&... args) {
  cxx::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  rexec(rpres.get_proc(), set_result_void<R>, rpres);
}
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
qthread::future<R> async(rlaunch policy, std::ptrdiff_t dest, F &&f,
                         Args &&... args) {
  if (dest == rank())
    return qthread::async(detail::local_policy(policy), std::forward<F>(f),
                          std::forward<Args>(args)...);
  auto pol = detail::decode_policy(policy);
  switch (pol) {
  case rlaunch::async:
  case rlaunch::sync: {
    auto pres = new qthread::promise<R>;
    auto fres = pres->get_future();
    rexec(dest, detail::continued<std::decay_t<F>, std::decay_t<Args>...>,
          rptr<qthread::promise<R>>(pres), std::forward<F>(f),
          std::forward<Args>(args)...);
    if (pol == rlaunch::sync)
      fres.wait();
    return fres;
  }
  case rlaunch::deferred: {
    return qthread::async(qthread::launch::deferred,
                          [dest](auto &&f, auto &&... args) {
                            return async(rlaunch::async, dest, std::move(f),
                                         std::move(args)...).get();
                          },
                          std::forward<F>(f), std::forward<Args>(args)...);
  }
  case rlaunch::detached: {
    rexec(dest, std::forward<F>(f), std::forward<Args>(args)...);
    return qthread::future<R>();
  }
  }
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
qthread::future<R> async(rlaunch policy,
                         qthread::future<std::ptrdiff_t> &&fdest, F &&f,
                         Args &&... args) {
  assert(fdest.valid());
  if (fdest.ready())
    return async(policy, fdest.get(), std::forward<F>(f),
                 std::forward<Args>(args)...);
  return qthread::async(
      detail::local_policy(policy),
      [fdest = std::move(fdest)](auto &&f, auto &&... args) mutable {
        return async(rlaunch::sync, fdest.get(), std::move(f),
                     std::move(args)...).get();
      },
      std::forward<F>(f), std::forward<Args>(args)...);
}
}

#define FUNHPC_ASYNC_HPP_DONE
#endif // #ifdef FUNHPC_ASYNC_HPP
#ifndef FUNHPC_ASYNC_HPP_DONE
#error "Cyclic include dependency"
#endif
