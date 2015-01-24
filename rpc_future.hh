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

template <typename T>
struct is_async<rpc::shared_future<T> > : std::true_type {};

// functor

template <typename F, typename T, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = cxx::invoke_of_t<F, T, As...> >
C<R> fmap(const F &f, const rpc::shared_future<T> &xs, const As &... as) {
  bool s = xs.valid();
  if (s == false)
    return rpc::shared_future<R>();
  return rpc::async([f, xs, as...]() {
                      return cxx::invoke(f, xs.get(), as...);
                    }).share();
}

template <typename F, typename T, typename T2, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = cxx::invoke_of_t<F, T, T2, As...> >
C<R> fmap2(const F &f, const rpc::shared_future<T> &xs,
           const rpc::shared_future<T2> &ys, const As &... as) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (s == false)
    return rpc::shared_future<R>();
  return rpc::async([f, xs, ys, as...]() {
                      return cxx::invoke(f, xs.get(), ys.get(), as...);
                    }).share();
}

template <typename F, typename T, typename T2, typename T3, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = cxx::invoke_of_t<F, T, T2, T3, As...> >
C<R> fmap3(const F &f, const rpc::shared_future<T> &xs,
           const rpc::shared_future<T2> &ys, const rpc::shared_future<T3> &zs,
           const As &... as) {
  bool s = xs.valid();
  assert(ys.valid() == s);
  assert(zs.valid() == s);
  if (s == false)
    return rpc::shared_future<R>();
  return rpc::async([f, xs, ys, zs, as...]() {
                      return cxx::invoke(f, xs.get(), ys.get(), zs.get(),
                                         as...);
                    }).share();
}

// foldable

template <typename Op, typename R, typename... As,
          typename CR = rpc::shared_future<R>,
          template <typename> class C = kinds<CR>::template constructor>
R fold(const Op &op, const R &z, const rpc::shared_future<R> &xs,
       const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, "");
  bool s = xs.valid();
  if (s == false)
    return z;
  return xs.get();
}

template <typename T> const T &head(const rpc::shared_future<T> &xs) {
  assert(xs.valid());
  return xs.get();
}
template <typename T> const T &last(const rpc::shared_future<T> &xs) {
  assert(xs.valid());
  return xs.get();
}

template <typename F, typename Op, typename R, typename T, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = kinds<CT>::template constructor>
R foldMap(const F &f, const Op &op, const R &z, const rpc::shared_future<T> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  if (s == false)
    return z;
  return cxx::invoke(f, xs.get(), as...);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As, typename CT = rpc::shared_future<T>,
          template <typename> class C = kinds<CT>::template constructor>
R foldMap2(const F &f, const Op &op, const R &z,
           const rpc::shared_future<T> &xs, const rpc::shared_future<T2> &ys,
           const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  bool s = xs.valid();
  assert(ys.valid() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, xs.get(), ys.get(), as...);
}

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus we
// cannot handle invalid futures as monads.

template <template <typename> class C, typename T>
std::enable_if_t<cxx::is_shared_future<C<T> >::value, C<T> > munit(const T &x) {
  return rpc::make_ready_future(x);
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_shared_future<C<T> >::value, C<T> >
mmake(const As &... as) {
  return rpc::make_ready_future(T(as...));
}

template <typename T, typename F, typename... As,
          typename CT = rpc::shared_future<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const rpc::shared_future<T> &xs, const F &f, const As &... as) {
  return rpc::future_then(xs, [f, as...](const rpc::shared_future<T> &xs) {
    return cxx::invoke(f, xs.get(), as...).get();
  });
}

template <typename T, typename CCT = rpc::shared_future<rpc::shared_future<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const rpc::shared_future<rpc::shared_future<T> > &xss) {
  return xss.unwrap();
}

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...>,
          std::enable_if_t<cxx::is_shared_future<C<T> >::value> * = nullptr>
auto iota(const F &f, const iota_range_t &range, const As &... as) {
  assert(range.local.size() == 1);
  return munit<C>(cxx::invoke(f, range.local.imin, as...));
}

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As,
          typename T = cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
          std::enable_if_t<cxx::is_shared_future<C<T> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  std::ptrdiff_t s = range.size();
  assert(s == 1);
  return munit<C>(cxx::invoke(f, global_range, range.imin(), as...));
}
}

#define RPC_FUTURE_HH_DONE
#else
#ifndef RPC_FUTURE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_FUTURE_HH
