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
// Round-robin load distribution
int choose_dest();
}
}

////////////////////////////////////////////////////////////////////////////////

namespace cxx {

// kinds
template <typename T> struct kinds<rpc::client<T> > {
  typedef T element_type;
  template <typename U> using constructor = rpc::client<U>;
};
template <typename T> struct is_client : std::false_type {};
template <typename T> struct is_client<rpc::client<T> > : std::true_type {};

// foldable

// TODO: remove special case for functions; expect actions
// everywhere. note this requires passing functions as values, not
// only as types.

template <typename R, typename T, typename F, typename CT = rpc::client<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    ((!rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(const F &f, const R &z, const rpc::client<T> &xs) {
  // Note: We could call make_local, but we don't
  return !xs ? z : cxx::invoke(f, z, *xs);
}

namespace client {
template <typename R, typename T, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(F, const R &z, const rpc::client<T> &xs) {
  return cxx::invoke(F(), z, *xs);
}
template <typename R, typename T, typename F>
struct foldl_action
    : public rpc::action_impl<
          foldl_action<R, T, F>,
          rpc::wrap<decltype(&foldl<R, T, F>), &foldl<R, T, F> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(cxx::rpc_client::foldl_action<R, T, F>::evaluate);
// RPC_CLASS_EXPORT(cxx::rpc_client::foldl_action<R, T, F>::finish);
}
template <typename R, typename T, typename F, typename CT = rpc::client<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    ((rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(F, const R &z, const rpc::client<T> &xs) {
  return !xs ? z : rpc::sync(rpc::remote::sync, xs.get_proc(),
                             client::foldl_action<R, T, F>(), F(), z, xs);
}

// functor
namespace detail {
template <typename T> struct unwrap_client {
  typedef T type;
  const T &make_local(const T &x) const { return x; }
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_client<rpc::client<T> > {
  typedef T type;
  rpc::client<T> make_local(const rpc::client<T> &x) const {
    return x.make_local();
  }
  const T &operator()(const rpc::client<T> &x) const { return *x; }
};

template <typename T, typename F, typename... As>
typename std::enable_if<
    rpc::is_action<F>::value,
    rpc::client<typename cxx::invoke_of<
        F, T, typename detail::unwrap_client<As>::type...>::type> >::type
fmap(F, const rpc::client<T> &xs, const As &... as) {
  typedef typename cxx::invoke_of<
      F, T, typename detail::unwrap_client<As>::type...>::type R;
  return rpc::make_client<R>(cxx::invoke(
      F(), *xs, detail::unwrap_client<As>()(
                    detail::unwrap_client<As>().make_local(as))...));
}
template <typename T, typename F, typename... As>
struct fmap_action
    : public rpc::action_impl<
          fmap_action<T, F, As...>,
          rpc::wrap<decltype(&fmap<T, F, As...>), &fmap<T, F, As...> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(fmap_action<T, F, As...>::evaluate);
// RPC_CLASS_EXPORT(fmap_action<T, F, As...>::finish);
}

template <typename T, typename... As, typename F, typename CT = rpc::client<T>,
          template <typename> class C = kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_client<As>::type...>::type>
typename std::enable_if<!rpc::is_action<F>::value, C<R> >::type
fmap(const F &f, const rpc::client<T> &xs, const As &... as) {
  return C<R>(rpc::async([
    f,
    xs,
    as...
  ]() {
     return rpc::make_client<R>(
         cxx::invoke(f, *xs, detail::unwrap_client<As>()(as)...));
   }));
}

template <typename T, typename... As, typename F, typename CT = rpc::client<T>,
          template <typename> class C = kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_client<As>::type...>::type>
typename std::enable_if<rpc::is_action<F>::value, C<R> >::type
fmap(F, const rpc::client<T> &xs, const As &... as) {
  return C<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                         detail::fmap_action<T, F, As...>(), F(), xs, as...));
}

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus we
// cannot handle empty clients as monads.

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

template <typename T, typename F, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
typename std::enable_if<!rpc::is_action<F>::value, C<R> >::type
bind(const rpc::client<T> &xs, const F &f) {
  return C<R>(rpc::async([xs, f]() { return cxx::invoke(f, *xs); }));
}

namespace detail {
template <typename T, typename F>
typename std::enable_if<rpc::is_action<F>::value,
                        typename cxx::invoke_of<F, T>::type>::type
bind(const rpc::client<T> &xs, F) {
  return cxx::invoke(F(), *xs);
}
template <typename T, typename F>
struct bind_action
    : public rpc::action_impl<bind_action<T, F>,
                              rpc::wrap<decltype(&bind<T, F>), &bind<T, F> > > {
};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(bind_action<M, R, T, F>::evaluate);
// RPC_CLASS_EXPORT(bind_action<M, R, T, F>::finish);
}
template <typename T, typename F, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
typename std::enable_if<rpc::is_action<F>::value, C<R> >::type
bind(const rpc::client<T> &xs, F) {
  return C<R>(rpc::async(rpc::remote::async, xs.get_proc_future(),
                         detail::bind_action<T, F>(), xs, F()));
}

// TODO: execute remotely
template <typename T, typename CCT = rpc::client<rpc::shared_future<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::element_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const rpc::client<rpc::client<T> > &xss) {
  return C<T>(rpc::async([xss]() { return *xss.make_local(); }));
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
