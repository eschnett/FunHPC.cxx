#ifndef RPC_MEMORY_HH
#define RPC_MEMORY_HH

// Use Boost as memory implementation

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace rpc {

using ::boost::make_shared;
using ::boost::shared_ptr;
}

#define RPC_MEMORY_HH_DONE
#else
#ifndef RPC_MEMORY_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_MEMORY_HH
