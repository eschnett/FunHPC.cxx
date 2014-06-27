#ifndef RPC_CALL_FWD_HH
#define RPC_CALL_FWD_HH

namespace rpc {

// Remote call policies

enum class remote : int {
  async = 1,
  deferred = 2,
  sync = 4,
  detached = 8
};

inline constexpr remote operator~(remote a) {
  return static_cast<remote>(~static_cast<int>(a));
}

inline constexpr remote operator&(remote a, remote b) {
  return static_cast<remote>(static_cast<int>(a) & static_cast<int>(b));
}
inline constexpr remote operator|(remote a, remote b) {
  return static_cast<remote>(static_cast<int>(a) | static_cast<int>(b));
}
inline constexpr remote operator^(remote a, remote b) {
  return static_cast<remote>(static_cast<int>(a) ^ static_cast<int>(b));
}

inline remote &operator&=(remote &a, remote b) { return a = a & b; }
inline remote &operator|=(remote &a, remote b) { return a = a | b; }
inline remote &operator^=(remote &a, remote b) { return a = a ^ b; }
}

#define RPC_CALL_FWD_HH_DONE
#else
#ifndef RPC_CALL_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CALL_FWD_HH
