#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include "cxx_invoke.hh"

#include "rpc_future.hh"

#include <type_traits>

namespace rpc {

// TODO: Implement "futurize"

extern mutex io_mutex;

template <typename M, typename F, typename... As>
auto with_lock(M &m, const F &f, As &&... args)
    -> typename cxx::invoke_of<F, As...>::type {
  lock_guard<decltype(m)> g(m);
  return cxx::invoke(f, std::forward<As>(args)...);
}

template <typename T, typename... As, typename F>
typename cxx::invoke_of<F, T &>::type
as_transaction(std::atomic<T> &var, const F &func, const As &... args) {
  typedef typename cxx::invoke_of<F, T &, As...>::type R;
  T current = var.load();
  for (;;) {
    T desired = current;
    R result = cxx::invoke(func, desired, args...);
    bool was_changed = var.compare_exchange_weak(current, desired);
    if (was_changed)
      return result;
  }
}

char *strdup(const char *str);

const char *make_hash_string(size_t hash_code);
const char *make_hash_string(const char *str);
}

#define RPC_DEFS_HH_DONE
#else
#ifndef RPC_DEFS_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_DEFS_HH
