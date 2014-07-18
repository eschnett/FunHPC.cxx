#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
#include "rpc_global_shared_ptr.hh"

#include "cxx_invoke.hh"

#include <cereal/archives/binary.hpp>

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rpc {

template <typename T>
T shared_future_get(const rpc::global_ptr<rpc::shared_future<T> > &ptr) {
  T res = ptr->get();
  delete ptr.get();
  return res;
}

template <typename T>
struct shared_future_get_action
    : public rpc::action_impl<
          shared_future_get_action<T>,
          rpc::wrap<decltype(&shared_future_get<T>), &shared_future_get<T> > > {
};
}

namespace cereal {

template <typename Archive, typename T>
inline void save(Archive &ar, const rpc::shared_future<T> &data) {
  bool ready = rpc::future_is_ready(data);
  ar(ready);
  if (ready) {
    // Send the data of the future
    ar(data.get());
  } else {
    // Create a global pointer to the future, and send it
    auto ptr = rpc::make_global<rpc::shared_future<T> >(data);
    ar(ptr);
  }
}

template <typename Archive, typename T>
inline void load(Archive &ar, rpc::shared_future<T> &data) {
  // RPC_ASSERT(!data.valid());
  bool ready;
  ar(ready);
  if (ready) {
    // Receive the data, and create a future from it
    T data_;
    ar(data_);
    data = rpc::make_ready_future(std::move(data_));
  } else {
    // Create a future that is waiting for the data of the remote
    // future
    rpc::global_ptr<rpc::shared_future<T> > ptr;
    ar(ptr);
    data = async(rpc::remote::async, ptr.get_proc(),
                 rpc::shared_future_get_action<T>(), ptr);
  }
}
}

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

namespace cxx {
namespace foldable {

// TODO: remove special case for functions; expect actions
// everywhere. note this requires passing functions as values, not
// only as types.

template <typename R, typename T, typename F>
typename std::enable_if<
    ((!rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(const F &f, const R &z, const rpc::client<T> &x) {
  // Note: We could call make_local, but we don't
  return !x ? z : cxx::invoke(f, z, *x);
}

namespace detail {
template <typename R, typename T, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(F, const R &z, const rpc::client<T> &x) {
  return cxx::invoke(F(), z, *x);
}
template <typename R, typename T, typename F>
struct foldl_action
    : public rpc::action_impl<
          foldl_action<R, T, F>,
          rpc::wrap<decltype(&foldl<R, T, F>), &foldl<R, T, F> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(foldl_action<R, T, F>::evaluate);
// RPC_CLASS_EXPORT(foldl_action<R, T, F>::finish);
}
template <typename R, typename T, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) &&
     (std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value)),
    R>::type
foldl(F, const R &z, const rpc::client<T> &x) {
  return !x ? z : rpc::sync(rpc::remote::sync, x.get_proc(),
                            detail::foldl_action<R, T, F>(), F(), z, x);
}
}

namespace functor {

namespace detail {
template <typename T> struct is_rpc_client : std::false_type {};
template <typename T> struct is_rpc_client<rpc::client<T> > : std::true_type {};
}

namespace detail {
template <typename T> struct unwrap_rpc_client {
  typedef T type;
  const T &make_local(const T &x) const { return x; }
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_rpc_client<rpc::client<T> > {
  typedef T type;
  rpc::client<T> make_local(const rpc::client<T> &x) const {
    return x.make_local();
  }
  const T &operator()(const rpc::client<T> &x) const { return *x; }
};

template <template <typename> class M, typename R, typename F, typename... As>
typename std::enable_if<
    ((rpc::is_action<F>::value) && (detail::is_rpc_client<M<R> >::value) &&
     (std::is_same<
         typename cxx::invoke_of<
             F, typename detail::unwrap_rpc_client<As>::type...>::type,
         R>::value)),
    M<R> >::type
fmap(F, const As &... as) {
  return M<R>(rpc::async([](const As &... as) {
                           return rpc::make_client<R>(cxx::invoke(
                               F(), detail::unwrap_rpc_client<As>()(as)...));
                         },
                         as...));
}
template <template <typename> class M, typename R, typename F, typename... As>
struct fmap_action
    : public rpc::action_impl<
          fmap_action<M, R, F, As...>,
          rpc::wrap<decltype(&fmap<M, R, F, As...>), &fmap<M, R, F, As...> > > {
};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(fmap_action<M, R, F, As...>::evaluate);
// RPC_CLASS_EXPORT(fmap_action<M, R, F, As...>::finish);
}

template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((!rpc::is_action<F>::value) && (detail::is_rpc_client<M<R> >::value) &&
     (std::is_same<
         typename cxx::invoke_of<
             F, typename detail::unwrap_rpc_client<As>::type...>::type,
         R>::value)),
    M<R> >::type
fmap(const F &f, const As &... as) {
  return M<R>(rpc::async([f](const As &... as) {
                           return rpc::make_client<R>(cxx::invoke(
                               f, detail::unwrap_rpc_client<As>()(as)...));
                         },
                         as...));
}

template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) && (detail::is_rpc_client<M<R> >::value) &&
     (!any<detail::is_rpc_client<As>::value...>::value) &&
     (std::is_same<
         typename cxx::invoke_of<
             F, typename detail::unwrap_rpc_client<As>::type...>::type,
         R>::value)),
    M<R> >::type
fmap(F, const As &... as) {
  return M<R>(rpc::async([](const As &... as) {
                           return rpc::make_client<R>(cxx::invoke(
                               F(), detail::unwrap_rpc_client<As>()(as)...));
                         },
                         as...));
}
template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) && (detail::is_rpc_client<M<R> >::value) &&
     (any<detail::is_rpc_client<As>::value...>::value) &&
     (std::is_same<
         typename cxx::invoke_of<
             F, typename detail::unwrap_rpc_client<As>::type...>::type,
         R>::value)),
    M<R> >::type
fmap(F, const As &... as) {
  constexpr size_t N = ffs<detail::is_rpc_client<As>::value...>::value;
  static_assert(N >= 0 && N < sizeof...(As), "internal error");
  typedef typename detail::unwrap_rpc_client<
      typename std::tuple_element<N, std::tuple<As...> >::type>::type T;
  const auto t = std::tie(as...);
  const M<T> &x = std::get<N>(t);
  return M<R>(rpc::async(rpc::remote::async, x.get_proc_future(),
                         detail::fmap_action<M, R, F, As...>(), F(), as...));
}
}

namespace monad {

using cxx::functor::fmap;

// Note: We cannot unwrap a future where the inner future is invalid. Thus we
// cannot handle empty clients as monads.

namespace detail {
template <typename T> struct is_rpc_client : std::false_type {};
template <typename T> struct is_rpc_client<rpc::client<T> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_rpc_client<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  // return rpc::make_client<typename std::decay<T>::type>(std::forward<T>(x));
  return rpc::make_remote_client<typename std::decay<T>::type>(
      rpc::detail::choose_dest(), std::forward<T>(x));
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_rpc_client<M<T> >::value, M<T> >::type
make(As &&... as) {
  // return rpc::make_client<T>(std::forward<As>(as)...);
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(),
                                    std::forward<As>(as)...);
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((!rpc::is_action<F>::value) && (detail::is_rpc_client<M<T> >::value) &&
     (std::is_same<typename cxx::invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  return M<R>(rpc::async([x, f]() { return cxx::invoke(f, *x); }));
}

namespace detail {
namespace client {
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) && (detail::is_rpc_client<M<T> >::value) &&
     (std::is_same<typename cxx::invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, F) {
  return cxx::invoke(F(), *x);
}
template <template <typename> class M, typename R, typename T, typename F>
struct bind_action
    : public rpc::action_impl<
          bind_action<M, R, T, F>,
          rpc::wrap<decltype(&bind<M, R, T, F>), &bind<M, R, T, F> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(bind_action<M, R, T, F>::evaluate);
// RPC_CLASS_EXPORT(bind_action<M, R, T, F>::finish);
}
}
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((rpc::is_action<F>::value) && (detail::is_rpc_client<M<T> >::value) &&
     (std::is_same<typename cxx::invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  return M<R>(rpc::async(rpc::remote::async, x.get_proc_future(),
                         detail::client::bind_action<M, R, T, F>(), x, F()));
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_rpc_client<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  return M<T>(rpc::async([x]() { return *x.make_local(); }));
}
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
