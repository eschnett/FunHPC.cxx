#ifndef RPC_ACTION_HH
#define RPC_ACTION_HH

#include "rpc_call.hh"
#include "rpc_defs.hh"
#include "rpc_client_fwd.hh"
#include "rpc_global_ptr_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"

// Note: <boost/mpi/packed_[io]archive.hpp> need to be included before
// using the macro BOOST_CLASS_EXPORT
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/utility/identity_type.hpp>

#include <typeinfo>

#define RPC_VA_ARG0(arg0, ...) arg0
#define RPC_VA_ARGS1(arg0, ...) __VA_ARGS__

// #define RPC_CLASS_EXPORT(T) BOOST_CLASS_EXPORT(T)
#define RPC_CLASS_EXPORT(T) RPC_CLASS_EXPORT_HASHED(T)

#define RPC_CLASS_EXPORT_HASHED(T)                                             \
  namespace boost {                                                            \
  namespace archive {                                                          \
  namespace detail {                                                           \
  namespace extra_detail {                                                     \
  template <> struct init_guid<T> {                                            \
    static guid_initializer<T> const &g;                                       \
  };                                                                           \
  guid_initializer<T> const &init_guid<T>::g =                                 \
      ::boost::serialization::singleton<                                       \
          guid_initializer<T> >::get_mutable_instance().export_guid();         \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  namespace boost {                                                            \
  namespace serialization {                                                    \
  template <> struct guid_defined<T> : boost::mpl::true_ {};                   \
  template <> inline const char *guid<T>() {                                   \
    static const char *guid_ = nullptr;                                        \
    if (!guid_) {                                                              \
      /*guid_ = rpc::make_hash_string(BOOST_PP_STRINGIZE(T));*/                \
      guid_ = rpc::make_hash_string(typeid(T).hash_code());                    \
    }                                                                          \
    return guid_;                                                              \
  }                                                                            \
  }                                                                            \
  }

#define RPC_DECLARE_ACTION(f)                                                  \
  struct f##_action                                                            \
      : public rpc::action_impl<f##_action, rpc::wrap<decltype(&f), &f> > {};

#define RPC_IMPLEMENT_ACTION(f)                                                \
  RPC_CLASS_EXPORT(f##_action::evaluate) RPC_CLASS_EXPORT(f##_action::finish)

#define RPC_ACTION(f) RPC_DECLARE_ACTION(f) RPC_IMPLEMENT_ACTION(f)

// Macro arguments: (c, ts...)
#define RPC_DECLARE_CONSTRUCTOR(...) /* do nothing */

#define RPC_IMPLEMENT_CONSTRUCTOR(...)                                         \
  RPC_CLASS_EXPORT(BOOST_IDENTITY_TYPE(                                        \
      (rpc::make_global_shared_action<__VA_ARGS__>::evaluate)))                \
      RPC_CLASS_EXPORT(BOOST_IDENTITY_TYPE(                                    \
          (rpc::make_global_shared_action<__VA_ARGS__>::finish)))

#define RPC_DECLARE_MEMBER_ACTION(c, f)                                        \
  struct f##_action : public rpc::member_action_impl<                          \
                          f##_action, rpc::wrap<decltype(&c::f), &c::f> > {};

#define RPC_IMPLEMENT_MEMBER_ACTION(c, f)                                      \
  RPC_CLASS_EXPORT(c::f##_action::evaluate)                                    \
      RPC_CLASS_EXPORT(c::f##_action::finish)

#define RPC_DECLARE_CONST_MEMBER_ACTION(c, f)                                  \
  struct f##_action : public rpc::const_member_action_impl<                    \
                          f##_action, rpc::wrap<decltype(&c::f), &c::f> > {};

#define RPC_IMPLEMENT_CONST_MEMBER_ACTION(c, f)                                \
  RPC_CLASS_EXPORT(c::f##_action::evaluate)                                    \
      RPC_CLASS_EXPORT(c::f##_action::finish)

#define RPC_DECLARE_COMPONENT(c) /* do nothing */

#define RPC_IMPLEMENT_COMPONENT(c)                                             \
  RPC_CLASS_EXPORT(rpc::global_ptr_get_action<c>::evaluate)                    \
      RPC_CLASS_EXPORT(rpc::global_ptr_get_action<c>::finish)                  \
      RPC_CLASS_EXPORT(rpc::make_global_shared_action<c>::evaluate)            \
      RPC_CLASS_EXPORT(rpc::make_global_shared_action<c>::finish)              \
      RPC_CLASS_EXPORT(rpc::shared_future_get_action<c>::evaluate)             \
      RPC_CLASS_EXPORT(rpc::shared_future_get_action<c>::finish)               \
      RPC_CLASS_EXPORT(                                                        \
          rpc::shared_future_get_action<rpc::global_shared_ptr<c> >::evaluate) \
      RPC_CLASS_EXPORT(                                                        \
          rpc::shared_future_get_action<rpc::global_shared_ptr<c> >::finish)

#define RPC_COMPONENT(c) RPC_DECLARE_COMPONENT(c) RPC_IMPLEMENT_COMPONENT(c)

#define RPC_ACTION_HH_DONE
#else
#ifndef RPC_ACTION_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_ACTION_HH
