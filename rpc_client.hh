#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
#include "rpc_future.hh"
#include "rpc_global_shared_ptr.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <cereal/archives/binary.hpp>

#include <array>
#include <type_traits>
#include <utility>

namespace rpc {

template <typename T>
template <class Archive>
void client<T>::serialize(Archive &ar) {
  ar(data);
}

template <typename T, typename... As>
client<T> make_remote_client(int proc, const As &... args) {
  return make_remote_client<T>(remote::async, proc, args...);
}

template <typename T, typename... As>
client<T> make_remote_client(const shared_future<int> &proc,
                             const As &... args) {
  return make_remote_client<T>(remote::async, proc, args...);
}

template <typename T, typename... As>
client<T> make_remote_client(remote policy, int proc, const As &... args) {
  return async(policy, proc, make_global_shared_action<T, As...>(), args...);
}

template <typename T, typename... As>
client<T> make_remote_client(remote policy, const shared_future<int> &proc,
                             const As &... args) {
  return async(policy, proc, make_global_shared_action<T, As...>(), args...);
}
}

namespace rpc {
namespace detail {
// Load distribution
int choose_dest();
}
}

////////////////////////////////////////////////////////////////////////////////

namespace cxx {

// kinds
template <typename T> struct kinds<rpc::client<T> > {
  typedef T value_type;
  template <typename U> using constructor = rpc::client<U>;
};
template <typename T> struct is_client : std::false_type {};
template <typename T> struct is_client<rpc::client<T> > : std::true_type {};

// foldable

// TODO: remove special case for functions; expect actions
// everywhere. note this requires passing functions as values, not
// only as types.

template <typename F, bool is_action, typename R, typename T, typename... As>
struct client_foldable;

template <typename F, typename R, typename T, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const rpc::client<T> &xs, const As &... as) {
  return client_foldable<F, rpc::is_action<F>::value, R, T, As...>::foldl(
      f, z, xs, as...);
}

template <typename F, typename R, typename T, typename... As>
struct client_foldable<F, false, R, T, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
      "");

  static R foldl_null(const F &f, const R &z, const rpc::client<T> &xs,
                      const As &... as) {
    return z;
  }

  static R foldl_client(const F &f, const R &z, const rpc::client<T> &xs,
                        const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    return cxx::invoke(f, z, *xs, as...);
  }

  static R foldl(const F &f, const R &z, const rpc::client<T> &xs,
                 const As &... as) {
    // Note: operator bool waits for the client to be ready
    bool s = bool(xs);
    return s == false ? foldl_null(f, z, xs, as...)
                      : foldl_client(f, z, xs, as...);
  }
};

template <typename F, typename R, typename T, typename... As>
struct client_foldable<F, true, R, T, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
      "");

  static R foldl_null(const R &z, const rpc::client<T> &xs, const As &... as) {
    return z;
  }

  static R foldl_client(const R &z, const rpc::client<T> &xs,
                        const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_client);
    return cxx::invoke(F(), z, *xs, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_client);

  static R foldl(F, const R &z, const rpc::client<T> &xs, const As &... as) {
    // Note: operator bool waits for the client to be ready
    bool s = bool(xs);
    return s == false ? foldl_null(z, xs, as...)
                      : rpc::sync(rpc::remote::sync, xs.get_proc(),
                                  foldl_client_action(), z, xs, as...);
  }
};
// Define action exports
template <typename F, typename R, typename T, typename... As>
typename client_foldable<F, true, R, T, As...>::foldl_client_evaluate_export_t
    client_foldable<F, true, R, T, As...>::foldl_client_evaluate_export =
        foldl_client_evaluate_export_init();
template <typename F, typename R, typename T, typename... As>
typename client_foldable<F, true, R, T, As...>::foldl_client_finish_export_t
    client_foldable<F, true, R, T, As...>::foldl_client_finish_export =
        foldl_client_finish_export_init();

template <typename F, bool is_action, typename R, typename T, typename T2,
          typename... As>
struct client_foldable2;

template <typename F, typename R, typename T, typename T2, typename... As,
          typename CT = rpc::client<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const rpc::client<T> &xs,
       const rpc::client<T2> &ys, const As &... as) {
  return client_foldable2<F, rpc::is_action<F>::value, R, T, T2, As...>::foldl2(
      f, z, xs, ys, as...);
}

template <typename F, typename R, typename T, typename T2, typename... As>
struct client_foldable2<F, false, R, T, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
      "");

  static R foldl2_null(const F &f, const R &z, const rpc::client<T> &xs,
                       const rpc::client<T2> &ys, const As &... as) {
    return z;
  }

  static R foldl2_client(const F &f, const R &z, const rpc::client<T> &xs,
                         const rpc::client<T2> &ys, const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    return cxx::invoke(f, z, *xs, *ys, as...);
  }

  static R foldl2(const F &f, const R &z, const rpc::client<T> &xs,
                  const rpc::client<T2> &ys, const As &... as) {
    // Note: operator bool waits for the client to be ready
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? foldl2_null(f, z, xs, ys, as...)
                      : foldl2_client(f, z, xs, ys, as...);
  }
};

template <typename F, typename R, typename T, typename T2, typename... As>
struct client_foldable2<F, true, R, T, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
      "");

  static R foldl2_null(const R &z, const rpc::client<T> &xs,
                       const rpc::client<T2> &ys, const As &... as) {
    return z;
  }

  static R foldl2_client(const R &z, const rpc::client<T> &xs,
                         const rpc::client<T2> &ys, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_client);
    return cxx::invoke(F(), z, *xs, *ys.make_local(), as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_client);

  static R foldl2(const F &f, const R &z, const rpc::client<T> &xs,
                  const rpc::client<T2> &ys, const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    // Note: operator bool waits for the client to be ready
    return s == false ? foldl2_null(z, xs, ys, as...)
                      : rpc::sync(rpc::remote::sync, xs.get_proc(),
                                  foldl2_client_action(), z, xs, ys, as...);
  }
};
// Define action exports
template <typename F, typename R, typename T, typename T2, typename... As>
typename client_foldable2<F, true, R, T, T2,
                          As...>::foldl2_client_evaluate_export_t
    client_foldable2<F, true, R, T, T2, As...>::foldl2_client_evaluate_export =
        foldl2_client_evaluate_export_init();
template <typename F, typename R, typename T, typename T2, typename... As>
typename client_foldable2<F, true, R, T, T2,
                          As...>::foldl2_client_finish_export_t
    client_foldable2<F, true, R, T, T2, As...>::foldl2_client_finish_export =
        foldl2_client_finish_export_init();

// functor

template <typename F, bool is_action, typename T, typename... As>
struct client_functor;

template <typename F, typename T, typename... As,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
rpc::client<R> fmap(const F &f, const rpc::client<T> &xs, const As &... as) {
  return client_functor<F, rpc::is_action<F>::value, T, As...>::fmap(f, xs,
                                                                     as...);
}

template <typename F, typename T, typename... As>
struct client_functor<F, false, T, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, As...>::type R;

  static rpc::client<R> fmap_client(const F &f, const rpc::client<T> &xs,
                                    const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    bool s = bool(xs);
    return s == false ? rpc::client<R>()
                      : rpc::make_client<R>(cxx::invoke(f, *xs, as...));
  }

  static rpc::client<R> fmap(const F &f, const rpc::client<T> &xs,
                             const As &... as) {
    return rpc::client<R>(async(fmap_client, f, xs, as...));
  }
};

template <typename F, typename T, typename... As>
struct client_functor<F, true, T, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, As...>::type R;

  static rpc::client<R> fmap_client(const rpc::client<T> &xs,
                                    const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client);
    bool s = bool(xs);
    return s == false ? rpc::client<R>()
                      : rpc::make_client<R>(cxx::invoke(F(), *xs, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client);

  static rpc::client<R> fmap(const F &f, const rpc::client<T> &xs,
                             const As &... as) {
    return rpc::client<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                                     fmap_client_action(), xs, as...));
  }
};
// Define action exports
template <typename F, typename T, typename... As>
typename client_functor<F, true, T, As...>::fmap_client_evaluate_export_t
client_functor<F, true, T, As...>::fmap_client_evaluate_export =
    fmap_client_evaluate_export_init();
template <typename F, typename T, typename... As>
typename client_functor<F, true, T, As...>::fmap_client_finish_export_t
client_functor<F, true, T, As...>::fmap_client_finish_export =
    fmap_client_finish_export_init();

template <typename F, bool is_action, typename T, typename T2, typename... As>
struct client_functor2;

template <typename F, typename T, typename T2, typename... As,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                     const rpc::client<T2> &ys, const As &... as) {
  return client_functor2<F, rpc::is_action<F>::value, T, T2, As...>::fmap2(
      f, xs, ys, as...);
}

template <typename F, typename T, typename T2, typename... As>
struct client_functor2<F, false, T, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;

  static rpc::client<R> fmap2_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? rpc::client<R>()
                      : rpc::make_client<R>(cxx::invoke(f, *xs, *ys, as...));
  }

  static rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys, const As &... as) {
    return rpc::client<R>(async(fmap2_client, f, xs, ys, as...));
  }
};

template <typename F, typename T, typename T2, typename... As>
struct client_functor2<F, true, T, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;

  static rpc::client<R> fmap2_client(const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client);
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? rpc::client<R>()
                      : rpc::make_client<R>(
                            cxx::invoke(F(), *xs, *ys.make_local(), as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client);

  static rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys, const As &... as) {
    return rpc::client<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                                     fmap2_client_action(), xs, ys, as...));
  }
};
// Define action exports
template <typename F, typename T, typename T2, typename... As>
typename client_functor2<F, true, T, T2, As...>::fmap2_client_evaluate_export_t
client_functor2<F, true, T, T2, As...>::fmap2_client_evaluate_export =
    fmap2_client_evaluate_export_init();
template <typename F, typename T, typename T2, typename... As>
typename client_functor2<F, true, T, T2, As...>::fmap2_client_finish_export_t
client_functor2<F, true, T, T2, As...>::fmap2_client_finish_export =
    fmap2_client_finish_export_init();

template <typename F, bool is_action, typename T, typename T2, typename T3,
          typename... As>
struct client_functor3;

template <typename F, typename T, typename T2, typename T3, typename... As,
          typename R = typename cxx::invoke_of<F, T, T2, T3, As...>::type>
rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                     const rpc::client<T2> &ys, const rpc::client<T3> &zs,
                     const As &... as) {
  return client_functor3<F, rpc::is_action<F>::value, T, T2, T3, As...>::fmap3(
      f, xs, ys, zs, as...);
}

template <typename F, typename T, typename T2, typename T3, typename... As>
struct client_functor3<F, false, T, T2, T3, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, T3, As...>::type R;

  static rpc::client<R> fmap3_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const rpc::client<T3> &zs,
                                     const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    bool s = bool(xs);
    assert(bool(ys) == s);
    assert(bool(zs) == s);
    return s == false
               ? rpc::client<R>()
               : rpc::make_client<R>(cxx::invoke(f, *xs, *ys, *zs, as...));
  }

  static rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys,
                              const rpc::client<T3> &zs, const As &... as) {
    return rpc::client<R>(async(fmap3_client, f, xs, ys, zs, as...));
  }
};

template <typename F, typename T, typename T2, typename T3, typename... As>
struct client_functor3<F, true, T, T2, T3, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, T2, T3, As...>::type R;

  static rpc::client<R> fmap3_client(const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const rpc::client<T3> &zs,
                                     const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client);
    bool s = bool(xs);
    assert(bool(ys) == s);
    assert(bool(zs) == s);
    return s == false
               ? rpc::client<R>()
               : rpc::make_client<R>(cxx::invoke(F(), *xs, *ys.make_local(),
                                                 *zs.make_local(), as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client);

  static rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys,
                              const rpc::client<T3> &zs, const As &... as) {
    return rpc::client<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                                     fmap3_client_action(), xs, ys, zs, as...));
  }
};
// Define action exports
template <typename F, typename T, typename T2, typename T3, typename... As>
typename client_functor3<F, true, T, T2, T3,
                         As...>::fmap3_client_evaluate_export_t
client_functor3<F, true, T, T2, T3, As...>::fmap3_client_evaluate_export =
    fmap3_client_evaluate_export_init();
template <typename F, typename T, typename T2, typename T3, typename... As>
typename client_functor3<F, true, T, T2, T3,
                         As...>::fmap3_client_finish_export_t
client_functor3<F, true, T, T2, T3, As...>::fmap3_client_finish_export =
    fmap3_client_finish_export_init();

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus
// we cannot handle empty clients as monads.

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
unit(T1 &&x) {
  // return rpc::make_client<T>(std::forward<T1>(x));
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(),
                                    std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
make(As &&... as) {
  // return rpc::make_client<T>(std::forward<As>(as)...);
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(),
                                    std::forward<As>(as)...);
}

namespace detail {
template <typename T, typename F, typename... As>
typename std::enable_if<rpc::is_action<F>::value,
                        typename cxx::invoke_of<F, T, As...>::type>::type
bind(const rpc::client<T> &xs, F, const As &... as) {
  return cxx::invoke(F(), *xs, as...);
}
template <typename T, typename F, typename... As>
struct bind_action
    : public rpc::action_impl<
          bind_action<T, F, As...>,
          rpc::wrap<decltype(&bind<T, F, As...>), &bind<T, F, As...> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(rpc::detail::bind_action<T, F, As...>::evaluate);
// RPC_CLASS_EXPORT(rpc::detail::bind_action<T, F, As...>::finish);
}

template <typename T, typename F, typename... As, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<!rpc::is_action<F>::value, C<R> >::type
bind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return C<R>(rpc::async([f](const rpc::client<T> &xs, const As &... as) {
                           return cxx::invoke(f, *xs, as...);
                         },
                         xs, as...));
}

template <typename T, typename F, typename... As, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<rpc::is_action<F>::value, C<R> >::type
bind(const rpc::client<T> &xs, F, const As &... as) {
  return C<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                         detail::bind_action<T, F, As...>(), xs, F(), as...));
}

// TODO: execute remotely
template <typename T, typename CCT = rpc::client<rpc::shared_future<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const rpc::client<rpc::client<T> > &xss) {
  return C<T>(rpc::async([xss]() { return *xss.make_local(); }));
}

// iota

// TODO: execute remotely?
template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  // return unit<C>(cxx::invoke(f, range.local.imin, as...));
  int dest =
      div_floor(rpc::server->size() * (range.local.imin - range.global.imin),
                range.global.imax - range.global.imin);
  return rpc::make_remote_client<T>(dest,
                                    cxx::invoke(f, range.local.imin, as...));
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
