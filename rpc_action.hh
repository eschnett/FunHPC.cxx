#ifndef RPC_ACTION_HH
#define RPC_ACTION_HH

#include "rpc_call.hh"
#include "rpc_client.hh"
#include "rpc_global_ptr.hh"
#include "rpc_shared_global_ptr.hh"



#define RPC_DECLARE_ACTION(f)                                           \
  struct f##_action:                                                    \
    public rpc::action_impl<f##_action, rpc::wrap<decltype(f), f>>      \
  {                                                                     \
  };

#define RPC_IMPLEMENT_ACTION(f)                 \
  BOOST_CLASS_EXPORT(f##_action::evaluate);     \
  BOOST_CLASS_EXPORT(f##_action::finish);

#define RPC_ACTION(f)                           \
  RPC_DECLARE_ACTION(f)                         \
  RPC_IMPLEMENT_ACTION(f)



#define RPC_DECLARE_MEMBER_ACTION(c, f)                                 \
  struct f##_action:                                                    \
    public rpc::member_action_impl<f##_action,                          \
                                   rpc::wrap<decltype(&c::f), &c::f>>   \
  {                                                                     \
  };

#define RPC_IMPLEMENT_MEMBER_ACTION(c, f)       \
  BOOST_CLASS_EXPORT(c::f##_action::evaluate);  \
  BOOST_CLASS_EXPORT(c::f##_action::finish);



#define RPC_DECLARE_CONST_MEMBER_ACTION(c, f)                           \
  struct f##_action:                                                    \
    public rpc::const_member_action_impl<f##_action,                    \
                                         rpc::wrap<decltype(&c::f), &c::f>> \
  {                                                                     \
  };

#define RPC_IMPLEMENT_CONST_MEMBER_ACTION(c, f) \
  BOOST_CLASS_EXPORT(c::f##_action::evaluate);  \
  BOOST_CLASS_EXPORT(c::f##_action::finish);



#define RPC_DECLARE_COMPONENT(c) /* do nothing */

// TODO: These are functions that take arguments, and only the default
// constructor is handled here
#define RPC_IMPLEMENT_COMPONENT(c)                                      \
  BOOST_CLASS_EXPORT(rpc::global_ptr_get_action<c>::evaluate);          \
  BOOST_CLASS_EXPORT(rpc::global_ptr_get_action<c>::finish);            \
  BOOST_CLASS_EXPORT(rpc::make_client_action<c>::evaluate);             \
  BOOST_CLASS_EXPORT(rpc::make_client_action<c>::finish);               \
  BOOST_CLASS_EXPORT(rpc::make_global_action<c>::evaluate);             \
  BOOST_CLASS_EXPORT(rpc::make_global_action<c>::finish);               \
  BOOST_CLASS_EXPORT(rpc::make_shared_global_action<c>::evaluate);      \
  BOOST_CLASS_EXPORT(rpc::make_shared_global_action<c>::finish);

#define RPC_COMPONENT(c)                        \
  RPC_DECLARE_COMPONENT(c)                      \
  RPC_IMPLEMENT_COMPONENT(c)



#define RPC_ACTION_HH_DONE
#else
#  ifndef RPC_ACTION_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_ACTION_HH
