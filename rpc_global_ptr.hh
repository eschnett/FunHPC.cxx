#ifndef RPC_GLOBAL_PTR_HH
#define RPC_GLOBAL_PTR_HH

#include "rpc_global_ptr_fwd.hh"

#include "rpc_call.hh"

namespace rpc {

// template <typename T> shared_ptr<T> global_ptr_get(const global_ptr<T> &ptr)
// {
//   return shared_ptr<T>(ptr.get());
// }
// template <typename T>
// struct global_ptr_get_action
//     : public rpc::action_impl<
//           global_ptr_get_action<T>,
//           rpc::wrap<decltype(&global_ptr_get<T>), &global_ptr_get<T> > > {};
//
// template <typename T> future<shared_ptr<T> > global_ptr<T>::make_local()
// const {
//   // Don't know whether to copy or not for local pointers
//   RPC_ASSERT(!is_local());
//   return async(rlaunch::async, get_proc(), global_ptr_get_action<T>(),
//   *this);
// }

template <typename T, typename... As>
struct make_global_action
    : public action_impl<
          make_global_action<T, As...>,
          wrap<decltype(&make_global<T, As...>), &make_global<T, As...> > > {};
}

#define RPC_GLOBAL_PTR_HH_DONE
#else
#ifndef RPC_GLOBAL_PTR_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_GLOBAL_PTR_HH
