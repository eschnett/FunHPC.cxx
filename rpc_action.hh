#ifndef RPC_ACTION_HH
#define RPC_ACTION_HH

#include "rpc_call.hh"
#include "rpc_component.hh"



#define RPC_ACTION(f)                                                   \
  struct f##_action:                                                    \
    public rpc::action_impl<f##_action, rpc::wrap<decltype(f), f>>      \
  {                                                                     \
  };                                                                    \
  BOOST_CLASS_EXPORT(f##_action::evaluate);                             \
  BOOST_CLASS_EXPORT(f##_action::finish);

#define RPC_MEMBER_ACTION(c, f)                                         \
  struct c##_##f##_action:                                              \
    public rpc::member_action_impl<c##_##f##_action,                    \
                                   rpc::wrap<decltype(&c::f), &c::f>>   \
  {                                                                     \
  };                                                                    \
  BOOST_CLASS_EXPORT(c##_##f##_action::evaluate);                       \
  BOOST_CLASS_EXPORT(c##_##f##_action::finish);

#define RPC_CONST_MEMBER_ACTION(c, f)                                   \
  struct c##_##f##_action:                                              \
    public rpc::const_member_action_impl<c##_##f##_action,              \
                                         rpc::wrap<decltype(&c::f), &c::f>> \
  {                                                                     \
  };                                                                    \
  BOOST_CLASS_EXPORT(c##_##f##_action::evaluate);                       \
  BOOST_CLASS_EXPORT(c##_##f##_action::finish);

#define RPC_COMPONENT(c)                                                \
  BOOST_CLASS_EXPORT(rpc::local_copy_helper_action<c>::evaluate);       \
  BOOST_CLASS_EXPORT(rpc::local_copy_helper_action<c>::finish);         \
  BOOST_CLASS_EXPORT(rpc::local_ptr_helper_action<c>::evaluate);        \
  BOOST_CLASS_EXPORT(rpc::local_ptr_helper_action<c>::finish);

#endif  // #ifndef RPC_ACTION_HH
