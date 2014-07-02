#ifndef RPC_MEMORY_HH
#define RPC_MEMORY_HH

#include <memory>

namespace rpc {

using ::std::make_shared;
using ::std::shared_ptr;
}

#define RPC_MEMORY_HH_DONE
#else
#ifndef RPC_MEMORY_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_MEMORY_HH
