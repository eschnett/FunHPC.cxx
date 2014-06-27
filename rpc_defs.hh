#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include "cxx_invoke.hh"

#include "rpc_future.hh"

namespace rpc {

// TODO: Implement "futurize"

extern mutex io_mutex;

template <typename M, typename F, typename... As>
auto with_lock(M &m, const F &f, As &&... args)
    -> typename invoke_of<F, As...>::type {
  lock_guard<decltype(m)> g(m);
  return invoke(f, std::forward<As>(args)...);
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
