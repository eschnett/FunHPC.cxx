#ifndef RPC_FUTURE_HH
#define RPC_FUTURE_HH

#include "rpc_future_fwd.hh"

#include "rpc_global_ptr.hh"

#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"

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
    data = async(rpc::rlaunch::async, ptr.get_proc(),
                 rpc::shared_future_get_action<T>(), ptr);
  }
}
}

////////////////////////////////////////////////////////////////////////////////

namespace cxx {

// kinds
template <typename T> struct kinds<rpc::shared_future<T> > {
  typedef T value_type;
  template <typename U> using constructor = rpc::shared_future<U>;
};
template <typename T> struct is_shared_future : std::false_type {};
template <typename T>
struct is_shared_future<rpc::shared_future<T> > : std::true_type {};

// foldable
template <typename F, typename R, typename T, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const rpc::shared_future<T> &xs,
      const As &... as) {
  bool s = xs.valid();
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.get(), as...);
}

template <typename F, typename R, typename T, typename T2, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const rpc::shared_future<T> &xs,
       const rpc::shared_future<T2> &ys, const As &... as) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.get(), ys.get(), as...);
}

// functor
template <typename F, typename T, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
C<R> fmap(const F &f, const rpc::shared_future<T> &xs, const As &... as) {
  bool s = xs.valid();
  if (s == false)
    return rpc::shared_future<R>();
  return rpc::async([f](const rpc::shared_future<T> &xs, const As &... as)
                        -> R { return cxx::invoke(f, xs.get(), as...); },
                    xs, as...).share();
}

template <typename F, typename T, typename T2, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap2(const F &f, const rpc::shared_future<T> &xs,
           const rpc::shared_future<T2> &ys, const As &... as) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (s == false)
    return rpc::shared_future<R>();
  return rpc::async(
             [f](const rpc::shared_future<T> xs,
                 const rpc::shared_future<T2> ys, const As &... as)
                 -> R { return cxx::invoke(f, xs.get(), ys.get(), as...); },
             xs, ys, as...).share();
}

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus we
// cannot handle invalid futures as monads.

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_shared_future<C<T> >::value, C<T> >::type
unit(T1 &&x) {
  return rpc::make_ready_future(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_shared_future<C<T> >::value, C<T> >::type
make(As &&... as) {
  return rpc::make_ready_future(T(std::forward<As>(as)...));
}

template <typename T, typename F, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> bind(const rpc::shared_future<T> &xs, const F &f, const As &... as) {
  return rpc::future_then(
      xs, [f](const rpc::shared_future<T> &xs, const As &... as) {
        return cxx::invoke(f, xs.get(), as...).get();
      });
}

template <typename T, typename CCT = rpc::shared_future<rpc::shared_future<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const rpc::shared_future<rpc::shared_future<T> > &xss) {
  return xss.unwrap();
}

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_shared_future<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  return unit<C>(cxx::invoke(f, range.local.imin, as...));
}
}

#define RPC_FUTURE_HH_DONE
#else
#ifndef RPC_FUTURE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_FUTURE_HH
