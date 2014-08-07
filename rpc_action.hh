#ifndef RPC_ACTION_HH
#define RPC_ACTION_HH

#include "rpc_call.hh"
#include "rpc_defs.hh"
#include "rpc_client_fwd.hh"
#include "rpc_global_ptr_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"

#include <cereal/archives/binary.hpp>
#include <cereal/types/polymorphic.hpp>

#include <functional>

#define RPC_VA_ARG0(arg0, ...) arg0
#define RPC_VA_ARGS1(arg0, ...) __VA_ARGS__

// Note: T needs to be passed surrounded by parentheses
#define RPC_IDENTITY_TYPE(T) typename std::function<void T>::argument_type

#define RPC_CLASS_EXPORT(T) CEREAL_REGISTER_TYPE(T)

#define RPC_DECLARE_ACTION(f)                                                  \
  struct f##_action                                                            \
      : public rpc::action_impl<f##_action, rpc::wrap<decltype(&f), &f> > {}

#define RPC_IMPLEMENT_ACTION(f)                                                \
  RPC_CLASS_EXPORT(f##_action::evaluate);                                      \
  RPC_CLASS_EXPORT(f##_action::finish)

#define RPC_ACTION(f)                                                          \
  RPC_DECLARE_ACTION(f);                                                       \
  RPC_IMPLEMENT_ACTION(f)

// Macro arguments: (c, ts...)
#define RPC_DECLARE_CONSTRUCTOR(...) /* do nothing */

#define RPC_IMPLEMENT_CONSTRUCTOR(...)                                         \
  RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE(                                          \
      (rpc::make_global_shared_action<__VA_ARGS__>::evaluate)));               \
  RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE(                                          \
      (rpc::make_global_shared_action<__VA_ARGS__>::finish)))

#define RPC_DECLARE_MEMBER_ACTION(c, f)                                        \
  struct f##_action : public rpc::member_action_impl<                          \
                          f##_action, rpc::wrap<decltype(&c::f), &c::f> > {}

#define RPC_IMPLEMENT_MEMBER_ACTION(c, f)                                      \
  RPC_CLASS_EXPORT(c::f##_action::evaluate);                                   \
  RPC_CLASS_EXPORT(c::f##_action::finish)

#define RPC_DECLARE_CONST_MEMBER_ACTION(c, f)                                  \
  struct f##_action : public rpc::const_member_action_impl<                    \
                          f##_action, rpc::wrap<decltype(&c::f), &c::f> > {}

#define RPC_IMPLEMENT_CONST_MEMBER_ACTION(c, f)                                \
  RPC_CLASS_EXPORT(c::f##_action::evaluate);                                   \
  RPC_CLASS_EXPORT(c::f##_action::finish)

#define RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(f)                           \
  RPC_DECLARE_ACTION(f);                                                       \
  typedef cereal::detail::bind_to_archives<                                    \
      typename f##_action::evaluate> const &f##_evaluate_export_t;             \
  static f##_evaluate_export_t f##_evaluate_export_init() {                    \
    return cereal::detail::StaticObject<cereal::detail::bind_to_archives<      \
        typename f##_action::evaluate> >::getInstance().bind();                \
  }                                                                            \
  static f##_evaluate_export_t f##_evaluate_export;                            \
  typedef cereal::detail::bind_to_archives<                                    \
      typename f##_action::finish> const &f##_finish_export_t;                 \
  static f##_finish_export_t f##_finish_export_init() {                        \
    return cereal::detail::StaticObject<cereal::detail::bind_to_archives<      \
        typename f##_action::finish> >::getInstance().bind();                  \
  }                                                                            \
  static f##_finish_export_t f##_finish_export

#define RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(f)                       \
  ((void)f##_evaluate_export, (void)f##_finish_export)

#define RPC_DECLARE_COMPONENT(c) /* do nothing */

#define RPC_IMPLEMENT_COMPONENT(c)                                             \
  RPC_CLASS_EXPORT(rpc::global_shared_ptr_get_action<c>::evaluate);            \
  RPC_CLASS_EXPORT(rpc::global_shared_ptr_get_action<c>::finish);              \
  RPC_CLASS_EXPORT(rpc::make_global_shared_action<c>::evaluate);               \
  RPC_CLASS_EXPORT(rpc::make_global_shared_action<c>::finish);                 \
  RPC_CLASS_EXPORT(                                                            \
      RPC_IDENTITY_TYPE((rpc::make_global_shared_action<c, c>::evaluate)));    \
  RPC_CLASS_EXPORT(                                                            \
      RPC_IDENTITY_TYPE((rpc::make_global_shared_action<c, c>::finish)));      \
  RPC_CLASS_EXPORT(rpc::shared_future_get_action<c>::evaluate);                \
  RPC_CLASS_EXPORT(rpc::shared_future_get_action<c>::finish);                  \
  RPC_CLASS_EXPORT(                                                            \
      rpc::shared_future_get_action<rpc::global_shared_ptr<c> >::evaluate);    \
  RPC_CLASS_EXPORT(                                                            \
      rpc::shared_future_get_action<rpc::global_shared_ptr<c> >::finish)

#define RPC_COMPONENT(c)                                                       \
  RPC_DECLARE_COMPONENT(c);                                                    \
  RPC_IMPLEMENT_COMPONENT(c)

#define RPC_ACTION_HH_DONE
#else
#ifndef RPC_ACTION_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_ACTION_HH
