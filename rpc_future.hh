#ifndef RPC_FUTURE_HH
#define RPC_FUTURE_HH

#include "rpc_future_fwd.hh"

#include "rpc_global_ptr.hh"

#include "cxx_invoke.hh"

#include <cereal/archives/binary.hpp>

#include <algorithm>
#include <type_traits>

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

// shared_future

namespace cxx {
namespace foldable {

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const rpc::shared_future<T> &x) {
  return !x.valid() ? z : cxx::invoke(f, z, x.get());
}
}

namespace functor {

namespace detail {
template <typename T> struct is_rpc_shared_future : std::false_type {};
template <typename T>
struct is_rpc_shared_future<rpc::shared_future<T> > : std::true_type {};
}

namespace detail {
template <typename T> struct unwrap_rpc_shared_future {
  typedef T type;
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_rpc_shared_future<rpc::shared_future<T> > {
  typedef T type;
  const T &operator()(const rpc::shared_future<T> &x) const { return x.get(); }
};
}
template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    ((detail::is_rpc_shared_future<M<R> >::value) &&
     (std::is_same<
         typename invoke_of<
             F, typename detail::unwrap_rpc_shared_future<As>::type...>::type,
         R>::value)),
    M<R> >::type
fmap(const F &f, const As &... as) {
  return rpc::async([f](const As &... as) {
                      return cxx::invoke(
                          f, detail::unwrap_rpc_shared_future<As>()(as)...);
                    },
                    as...).share();
}
}

namespace monad {

using cxx::functor::fmap;

// Note: We cannot unwrap a future where the inner future is invalid. Thus we
// cannot handle invalid futures as monads.

namespace detail {
template <typename T> struct is_rpc_shared_future : std::false_type {};
template <typename T>
struct is_rpc_shared_future<rpc::shared_future<T> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_rpc_shared_future<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return rpc::make_ready_future<T>(std::forward<T>(x)).share();
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_rpc_shared_future<M<T> >::value, M<T> >::type
make(As &&... as) {
  return rpc::make_ready_future<T>(T(std::forward<As>(as)...));
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_rpc_shared_future<M<T> >::value) &&
     (std::is_same<typename invoke_of<F, T>::type, M<R> >::value)),
    M<R> >::type
bind(const M<T> &x, const F &f) {
  return rpc::future_then(
      x, [f](const M<T> &x) { return cxx::invoke(f, x.get()).get(); });
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_rpc_shared_future<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  return x.unwrap();
}
}
}

#define RPC_FUTURE_HH_DONE
#else
#ifndef RPC_FUTURE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_FUTURE_HH
