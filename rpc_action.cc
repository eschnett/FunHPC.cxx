#include "rpc_action.hh"
#include "rpc_future.hh"
#include "rpc_global_shared_ptr.hh"

#include <cstdlib>

RPC_COMPONENT(std::ptrdiff_t);

// Copy constructor
namespace rpc {
typedef make_global_shared_action<std::ptrdiff_t, std::ptrdiff_t>::evaluate
make_global_shared_action_ptrdiff_t_evaluate;
typedef make_global_shared_action<std::ptrdiff_t, std::ptrdiff_t>::finish
make_global_shared_action_ptrdiff_t_finish;
}
RPC_CLASS_EXPORT(rpc::make_global_shared_action_ptrdiff_t_evaluate);
RPC_CLASS_EXPORT(rpc::make_global_shared_action_ptrdiff_t_finish);
