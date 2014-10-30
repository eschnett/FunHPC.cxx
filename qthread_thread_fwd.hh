#ifndef QTHREAD_THREAD_FWD_HH
#define QTHREAD_THREAD_FWD_HH

#include "qthread_future_fwd.hh"

#include "cxx_invoke.hh"
#include "cxx_utils.hh"

namespace qthread {

enum class launch : int { async = 1, deferred = 2, sync = 4 };

inline constexpr launch operator~(launch a) {
  return static_cast<launch>(~static_cast<int>(a));
}

inline constexpr launch operator&(launch a, launch b) {
  return static_cast<launch>(static_cast<int>(a) & static_cast<int>(b));
}
inline constexpr launch operator|(launch a, launch b) {
  return static_cast<launch>(static_cast<int>(a) | static_cast<int>(b));
}
inline constexpr launch operator^(launch a, launch b) {
  return static_cast<launch>(static_cast<int>(a) ^ static_cast<int>(b));
}

inline launch &
operator&=(launch &a, launch b) {
  return a = a & b;
}
inline launch &operator|=(launch &a, launch b) { return a = a | b; }
inline launch &operator^=(launch &a, launch b) { return a = a ^ b; }

template <typename F, typename... As>
auto async(launch policy, const F &func, As &&... args)
    -> typename std::enable_if<
        !std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
        future<typename cxx::invoke_of<F, As...>::type> >::type;

template <typename F, typename... As>
auto async(launch policy, const F &func, As &&... args)
    -> typename std::enable_if<
        std::is_void<typename cxx::invoke_of<F, As...>::type>::value,
        future<typename cxx::invoke_of<F, As...>::type> >::type;

// template<typename F, typename... As>
// auto async(launch policy, const F& func, As&&... args) ->
//   future<typename cxx::invoke_of<F, As...>::type>;

template <typename F, typename... As>
auto async(const F &func, As &&... args)
    -> future<typename cxx::invoke_of<F, As...>::type>;
}

#define QTHREAD_THREAD_FWD_HH_DONE
#else
#ifndef QTHREAD_THREAD_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // QTHREAD_THREAD_FWD_HH
