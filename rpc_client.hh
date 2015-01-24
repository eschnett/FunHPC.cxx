#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_action.hh"
#include "rpc_call.hh"
#include "rpc_future.hh"
#include "rpc_global_shared_ptr.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_shape.hh"

#include <cereal/archives/binary.hpp>

#include <cxxabi.h>

#include <cassert>
#include <iterator>
#include <type_traits>
#include <utility>

namespace rpc {

template <typename T>
template <class Archive>
void client<T>::serialize(Archive &ar) {
  ar(proc, data);
}

template <typename T, typename... As>
client<T> make_remote_client(int proc, const As &... args) {
  return make_remote_client<T>(rlaunch::async, proc, args...);
}

template <typename T, typename... As>
client<T> make_remote_client(const shared_future<int> &proc,
                             const As &... args) {
  return make_remote_client<T>(rlaunch::async, proc, args...);
}

template <typename T, typename... As>
client<T> make_remote_client(rlaunch policy, int proc, const As &... args) {
  return client<T>(proc, async(policy, proc,
                               make_global_shared_action<T, As...>(), args...));
}

template <typename T, typename... As>
client<T> make_remote_client(rlaunch policy, const shared_future<int> &proc,
                             const As &... args) {
  return async(policy, proc, make_global_shared_action<T, As...>(), args...);
}

template <typename F, typename... As>
client<cxx::invoke_of_t<F, As...> > local_client(rpc::launch policy, const F &f,
                                                 const As &... args) {
  static_assert(!rpc::is_action<F>::value, "");
  typedef cxx::invoke_of_t<F, As...> R;
  return client<R>(rpc::server->rank(),
                   async(policy, [&f, &args...]() {
                     return make_client<R>(cxx::invoke(f, args...));
                   }));
}

template <typename F, typename... As>
auto remote_client(int proc, const F &f, const As &... args) {
  return remote_client(rlaunch::async, proc, f, args...);
}

template <typename F, typename... As> struct call_and_make_client {
  typedef cxx::invoke_of_t<F, As...> R;
  static client<R> call(const F &f, const As &... args) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(call);
    return make_client<R>(cxx::invoke(f, args...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(call);
};
// Define action exports
template <typename F, typename... As>
typename call_and_make_client<F, As...>::call_evaluate_export_t
    call_and_make_client<F, As...>::call_evaluate_export =
        call_evaluate_export_init();
template <typename F, typename... As>
typename call_and_make_client<F, As...>::call_finish_export_t
    call_and_make_client<F, As...>::call_finish_export =
        call_finish_export_init();

template <typename F, typename... As>
client<cxx::invoke_of_t<F, As...> >
remote_client(rpc::rlaunch policy, int proc, const F &f, const As &... args) {
  static_assert(rpc::is_action<F>::value, "");
  typedef cxx::invoke_of_t<F, As...> R;
  return client<R>(proc,
                   async(policy, proc,
                         typename call_and_make_client<F, As...>::call_action(),
                         f, args...));
}
}

namespace rpc {
namespace detail {
// Load distribution
int choose_dest();
}
}

////////////////////////////////////////////////////////////////////////////////

namespace rpc {

// Iterator

template <typename T> struct client_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef T *pointer_type;
  typedef T &reference_type;
  typedef std::random_access_iterator_tag iterator_category;
  client<T> &c;
  std::ptrdiff_t i;
  client_iterator(client<T> &c, std::ptrdiff_t i = 0) : c(c), i(i) {}
  reference_type operator*() {
    assert(i == 0);
    return (&*c)[i];
  }
  client_iterator &operator++() {
    ++i;
    return *this;
  }
  bool operator==(const client_iterator<T> &it) const {
    return &c == &it.c && i == it.i;
  }
  bool operator!=(const client_iterator<T> &it) const { return !(*this == it); }
  client_iterator operator++(int) {
    client_iterator r(*this);
    ++*this;
    return r;
  }
  client_iterator &operator--() {
    --i;
    return *this;
  }
  client_iterator operator--(int) {
    client_iterator r(*this);
    --*this;
    return r;
  }
  client_iterator &operator+=(difference_type n) {
    i += n;
    return *this;
  }
  client_iterator operator+(difference_type n) const {
    client_iterator r(*this);
    r += n;
    return r;
  }
  client_iterator &operator-=(difference_type n) {
    i -= n;
    return *this;
  }
  client_iterator operator-(difference_type n) const {
    client_iterator r(*this);
    r -= n;
    return r;
  }
  difference_type operator-(const client_iterator &it) const {
    assert(&c == &it.c);
    return it.i - i;
  }
  reference_type operator[](difference_type n) { return *(*this + n); }
  bool operator<(const client_iterator &it) const { return (it - *this) > 0; }
  bool operator>(const client_iterator &it) const { return it < *this; }
  bool operator<=(const client_iterator &it) const { return !(*this < it); }
  bool operator>=(const client_iterator &it) const { return !(*this > it); }
};

template <typename T> struct const_client_iterator {
  typedef std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef const T *pointer_type;
  typedef const T &reference_type;
  typedef std::random_access_iterator_tag iterator_category;
  const client<T> &c;
  std::ptrdiff_t i;
  const_client_iterator(const client<T> &c, std::ptrdiff_t i = 0)
      : c(c), i(i) {}
  reference_type operator*() const {
    // TODO: We don't like the call to make_local here, but iterators
    // are process-local
    assert(i == 0);
    // return (&*c)[i];
    return *c.make_local();
  }
  const_client_iterator &operator++() {
    ++i;
    return *this;
  }
  bool operator==(const const_client_iterator<T> &it) const {
    return &c == &it.c && i == it.i;
  }
  bool operator!=(const const_client_iterator<T> &it) const {
    return !(*this == it);
  }
  const_client_iterator operator++(int) {
    const_client_iterator r(*this);
    ++*this;
    return r;
  }
  const_client_iterator &operator--() {
    --i;
    return *this;
  }
  const_client_iterator operator--(int) {
    const_client_iterator r(*this);
    --*this;
    return r;
  }
  const_client_iterator &operator+=(difference_type n) {
    i += n;
    return *this;
  }
  const_client_iterator operator+(difference_type n) const {
    const_client_iterator r(*this);
    r += n;
    return r;
  }
  const_client_iterator &operator-=(difference_type n) {
    i -= n;
    return *this;
  }
  const_client_iterator operator-(difference_type n) const {
    const_client_iterator r(*this);
    r -= n;
    return r;
  }
  difference_type operator-(const const_client_iterator &it) const {
    assert(&c == &it.c);
    return it.i - i;
  }
  reference_type operator[](difference_type n) const { return *(*this + n); }
  bool operator<(const const_client_iterator &it) const {
    return (it - *this) > 0;
  }
  bool operator>(const const_client_iterator &it) const { return it < *this; }
  bool operator<=(const const_client_iterator &it) const {
    return !(*this < it);
  }
  bool operator>=(const const_client_iterator &it) const {
    return !(*this > it);
  }
};
}

namespace std {
template <typename T> struct iterator_traits<rpc::client_iterator<T> > {
  typedef typename rpc::client_iterator<T>::difference_type difference_type;
  typedef typename rpc::client_iterator<T>::value_type value_type;
  typedef typename rpc::client_iterator<T>::pointer_type pointer_type;
  typedef typename rpc::client_iterator<T>::reference_type reference_type;
  typedef typename rpc::client_iterator<T>::iterator_category iterator_category;
};

template <typename T, typename Distance>
void advance(rpc::client_iterator<T> &it, Distance n) {
  it += n;
}
template <typename T>
typename std::iterator_traits<rpc::client_iterator<T> >::difference_type
distance(const rpc::client_iterator<T> &first,
         const rpc::client_iterator<T> &last) {
  return last - first;
}

template <typename T>
rpc::client_iterator<T> next(
    rpc::client_iterator<T> it,
    typename std::iterator_traits<rpc::client_iterator<T> >::difference_type n =
        1) {
  std::advance(it, n);
  return it;
}
template <typename T>
rpc::client_iterator<T> prev(
    rpc::client_iterator<T> it,
    typename std::iterator_traits<rpc::client_iterator<T> >::difference_type n =
        1) {
  std::advance(it, -n);
  return it;
}

template <typename T> rpc::client_iterator<T> begin(rpc::client<T> &c) {
  return rpc::client_iterator<T>(c);
}
template <typename T> rpc::client_iterator<T> end(rpc::client<T> &c) {
  return rpc::client_iterator<T>(c, 1);
}

template <typename T> struct iterator_traits<rpc::const_client_iterator<T> > {
  typedef typename rpc::const_client_iterator<T>::difference_type
      difference_type;
  typedef typename rpc::const_client_iterator<T>::value_type value_type;
  typedef typename rpc::const_client_iterator<T>::pointer_type pointer_type;
  typedef typename rpc::const_client_iterator<T>::reference_type reference_type;
  typedef typename rpc::const_client_iterator<T>::iterator_category
      iterator_category;
};

template <typename T, typename Distance,
          typename IT = rpc::const_client_iterator<T> >
void advance(IT &it, Distance n) {
  it += n;
}
template <typename T, typename IT = rpc::const_client_iterator<T> >
typename std::iterator_traits<IT>::difference_type distance(const IT &first,
                                                            const IT &last) {
  return last - first;
}

template <typename T>
rpc::const_client_iterator<T>
next(rpc::const_client_iterator<T> it,
     typename std::iterator_traits<
         rpc::const_client_iterator<T> >::difference_type n = 1) {
  std::advance(it, n);
  return it;
}
template <typename T>
rpc::const_client_iterator<T>
prev(rpc::const_client_iterator<T> it,
     typename std::iterator_traits<
         rpc::const_client_iterator<T> >::difference_type n = 1) {
  std::advance(it, -n);
  return it;
}

template <typename T>
rpc::const_client_iterator<T> begin(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c);
}
template <typename T>
rpc::const_client_iterator<T> cbegin(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c);
}
template <typename T>
rpc::const_client_iterator<T> end(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c, 1);
}
template <typename T>
rpc::const_client_iterator<T> cend(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c, 1);
}
}

namespace rpc {
template <typename T>
rpc::const_client_iterator<T> begin(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c);
}
template <typename T>
rpc::const_client_iterator<T> end(const rpc::client<T> &c) {
  return rpc::const_client_iterator<T>(c, 1);
}
}

namespace cxx {

// kinds
template <typename T> struct kinds<rpc::client<T> > {
  typedef T value_type;
  template <typename U> using constructor = rpc::client<U>;
};
template <typename T> struct is_client : std::false_type {};
template <typename T> struct is_client<rpc::client<T> > : std::true_type {};

template <typename T> struct is_async<rpc::client<T> > : std::true_type {};

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...> >
std::enable_if_t<(cxx::is_client<C<T> >::value && !rpc::is_action<F>::value),
                 C<T> >
iota(const F &f, const iota_range_t &range, const As &... as) {
  assert(range.local.size() == 1);
  return rpc::local_client(rpc::launch::async, f, range.local.imin, as...);
}

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...> >
std::enable_if_t<(cxx::is_client<C<T> >::value && rpc::is_action<F>::value),
                 C<T> >
iota(const F &f, const iota_range_t &range, const As &... as) {
  assert(range.local.size() == 1);
  int dest =
      div_floor(rpc::server->size() * (range.local.imin - range.global.imin),
                range.global.imax - range.global.imin);
  return rpc::remote_client(dest, f, range.local.imin, as...);
}

template <
    template <typename> class C, typename F, std::ptrdiff_t D, typename... As,
    typename T = cxx::invoke_of_t<F, cxx::grid_region<D>, cxx::index<D>, As...>,
    std::enable_if_t<cxx::is_client<C<T> >::value> * = nullptr>
auto iota(const F &f, const cxx::grid_region<D> &global_range,
          const cxx::grid_region<D> &range, const As &... as) {
  assert(range.size() == 1);
  cxx::grid_region<D> coarse_range(
      global_range.imin(), align_ceil(global_range.imax(), range.istep()),
      range.istep());
  ptrdiff_t idx = coarse_range.linear(range.imin());
  ptrdiff_t idx_max = coarse_range.linear_max();
  int dest = div_floor(rpc::server->size() * idx, idx_max);
  return rpc::remote_client(dest, f, global_range, range.imin(), as...);
}

// functor

template <typename F, bool is_action, typename T, typename... As>
struct client_functor;

template <typename F, typename T, typename... As,
          typename R = cxx::invoke_of_t<F, T, As...> >
rpc::client<R> fmap(const F &f, const rpc::client<T> &xs, const As &... as) {
  return client_functor<std::decay_t<F>, rpc::is_action<F>::value, T,
                        As...>::fmap(f, xs, as...);
}

template <typename F, typename T, typename... As>
struct client_functor<F, false, T, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, As...> R;

  static rpc::global_shared_ptr<R>
  fmap_client_local(const F &f, const rpc::client<T> &xs, const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, as...));
  }

  static rpc::client<R> fmap_client(const F &f, const rpc::client<T> &xs,
                                    const As &... as) {
    bool s = bool(xs);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(rpc::server->rank(),
                          rpc::async(fmap_client_local, f, xs, as...));
  }

  static rpc::client<R> fmap(const F &f, const rpc::client<T> &xs,
                             const As &... as) {
    return rpc::client<R>(rpc::async(fmap_client, f, xs, as...));
  }
};

template <typename F, typename T, typename... As>
struct client_functor<F, true, T, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, As...> R;

  static rpc::global_shared_ptr<R>
  fmap_client_remote(const F &f, const rpc::client<T> &xs, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client_remote);
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client_remote);

  static rpc::client<R> fmap_client(const F &f, const rpc::client<T> &xs,
                                    const As &... as) {
    bool s = bool(xs);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(
        xs.get_proc(), rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                  fmap_client_remote_action(), f, xs, as...));
  }

  static rpc::client<R> fmap(const F &f, const rpc::client<T> &xs,
                             const As &... as) {
    return rpc::client<R>(rpc::async(fmap_client, f, xs, as...));
  }
};
// Define action exports
template <typename F, typename T, typename... As>
typename client_functor<F, true, T, As...>::fmap_client_remote_evaluate_export_t
    client_functor<F, true, T, As...>::fmap_client_remote_evaluate_export =
        fmap_client_remote_evaluate_export_init();
template <typename F, typename T, typename... As>
typename client_functor<F, true, T, As...>::fmap_client_remote_finish_export_t
    client_functor<F, true, T, As...>::fmap_client_remote_finish_export =
        fmap_client_remote_finish_export_init();

template <typename F, bool is_action, typename T, typename T2, typename... As>
struct client_functor2;

template <typename F, typename T, typename T2, typename... As,
          typename R = cxx::invoke_of_t<F, T, T2, As...> >
rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                     const rpc::client<T2> &ys, const As &... as) {
  return client_functor2<std::decay_t<F>, rpc::is_action<F>::value, T, T2,
                         As...>::fmap2(f, xs, ys, as...);
}

template <typename F, typename T, typename T2, typename... As>
struct client_functor2<F, false, T, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, T2, As...> R;

  static rpc::global_shared_ptr<R> fmap2_client_local(const F &f,
                                                      const rpc::client<T> &xs,
                                                      const rpc::client<T2> &ys,
                                                      const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, *ys, as...));
  }

  static rpc::client<R> fmap2_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(rpc::server->rank(),
                          rpc::async(fmap2_client_local, f, xs, ys, as...));
  }

  static rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys, const As &... as) {
    return rpc::client<R>(rpc::async(fmap2_client, f, xs, ys, as...));
  }
};

template <typename F, typename T, typename T2, typename... As>
struct client_functor2<F, true, T, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, T2, As...> R;

  static rpc::global_shared_ptr<R> fmap2_client_remote(const F &f,
                                                       const rpc::client<T> &xs,
                                                       const rpc::client<T> &ys,
                                                       const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client_remote);
    auto ysl = ys.make_local();
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, *ysl, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client_remote);

  static rpc::client<R> fmap2_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(xs.get_proc(),
                          rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                     fmap2_client_remote_action(), f, xs, ys,
                                     as...));
  }

  static rpc::client<R> fmap2(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys, const As &... as) {
    return rpc::client<R>(rpc::async(fmap2_client, f, xs, ys, as...));
  }
};
// Define action exports
template <typename F, typename T, typename T2, typename... As>
typename client_functor2<F, true, T, T2,
                         As...>::fmap2_client_remote_evaluate_export_t
    client_functor2<F, true, T, T2,
                    As...>::fmap2_client_remote_evaluate_export =
        fmap2_client_remote_evaluate_export_init();
template <typename F, typename T, typename T2, typename... As>
typename client_functor2<F, true, T, T2,
                         As...>::fmap2_client_remote_finish_export_t
    client_functor2<F, true, T, T2, As...>::fmap2_client_remote_finish_export =
        fmap2_client_remote_finish_export_init();

template <typename F, bool is_action, typename T, typename T2, typename T3,
          typename... As>
struct client_functor3;

template <typename F, typename T, typename T2, typename T3, typename... As,
          typename R = cxx::invoke_of_t<F, T, T2, T3, As...> >
rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                     const rpc::client<T2> &ys, const rpc::client<T3> &zs,
                     const As &... as) {
  return client_functor3<std::decay_t<F>, rpc::is_action<F>::value, T, T2, T3,
                         As...>::fmap3(f, xs, ys, zs, as...);
}

template <typename F, typename T, typename T2, typename T3, typename... As>
struct client_functor3<F, false, T, T2, T3, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;

  static rpc::global_shared_ptr<R> fmap3_client_local(const F &f,
                                                      const rpc::client<T> &xs,
                                                      const rpc::client<T2> &ys,
                                                      const rpc::client<T3> &zs,
                                                      const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, *ys, *zs, as...));
  }

  static rpc::client<R> fmap3_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const rpc::client<T3> &zs,
                                     const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    assert(bool(zs) == s);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(rpc::server->rank(),
                          rpc::async(fmap3_client_local, f, xs, ys, zs, as...));
  }

  static rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys,
                              const rpc::client<T3> &zs, const As &... as) {
    return rpc::client<R>(rpc::async(fmap3_client, f, xs, ys, zs, as...));
  }
};

template <typename F, typename T, typename T2, typename T3, typename... As>
struct client_functor3<F, true, T, T2, T3, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;

  static rpc::global_shared_ptr<R>
  fmap3_client_remote(const F &f, const rpc::client<T> &xs,
                      const rpc::client<T2> &ys, const rpc::client<T3> &zs,
                      const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client_remote);
    auto ysl = ys.make_local();
    auto zsl = zs.make_local();
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, *ysl, *zsl, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client_remote);

  static rpc::client<R> fmap3_client(const F &f, const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const rpc::client<T3> &zs,
                                     const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    assert(bool(zs) == s);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(xs.get_proc(),
                          rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                     fmap3_client_remote_action(), f, xs, ys,
                                     zs, as...));
  }

  static rpc::client<R> fmap3(const F &f, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys,
                              const rpc::client<T3> &zs, const As &... as) {
    return rpc::client<R>(rpc::async(fmap3_client, f, xs, ys, zs, as...));
  }
};
// Define action exports
template <typename F, typename T, typename T2, typename T3, typename... As>
typename client_functor3<F, true, T, T2, T3,
                         As...>::fmap3_client_remote_evaluate_export_t
    client_functor3<F, true, T, T2, T3,
                    As...>::fmap3_client_remote_evaluate_export =
        fmap3_client_remote_evaluate_export_init();
template <typename F, typename T, typename T2, typename T3, typename... As>
typename client_functor3<F, true, T, T2, T3,
                         As...>::fmap3_client_remote_finish_export_t
    client_functor3<F, true, T, T2, T3,
                    As...>::fmap3_client_remote_finish_export =
        fmap3_client_remote_finish_export_init();

// TODO: Use Monad.sequence instead, assuming T is Traversible
template <typename F, bool is_action, typename T, typename B, std::ptrdiff_t D,
          typename... As>
struct client_functor_boundaries;

template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As,
          typename R = cxx::invoke_of_t<F, T, cxx::boundaries<B, D>, As...> >
rpc::client<R> fmap_boundaries(const F &f, const rpc::client<T> &xs,
                               const cxx::boundaries<rpc::client<B>, D> &bss,
                               const As &... as) {
  return client_functor_boundaries<std::decay_t<F>, rpc::is_action<F>::value, T,
                                   B, D, As...>::fmap_boundaries(f, xs, bss,
                                                                 as...);
}

template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
struct client_functor_boundaries<F, false, T, B, D, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, cxx::boundaries<B, D>, As...> R;

  static rpc::global_shared_ptr<R>
  fmap_boundaries_client_local(const F &f, const rpc::client<T> &xs,
                               const cxx::boundaries<rpc::client<B>, D> &bss,
                               const As &... as) {
    // Note: We could call make_local, but we don't want to for non-actions
    auto newbss = cxx::fmap([](const auto &bs) { return *bs; }, bss);
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, newbss, as...));
  }

  static rpc::client<R>
  fmap_boundaries_client(const F &f, const rpc::client<T> &xs,
                         const cxx::boundaries<rpc::client<B>, D> &bss,
                         const As &... as) {
    bool s = bool(xs);
    assert(all_of(fmap([s](const auto &bs) { return bool(bs) == s; }, bss)));
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(
        rpc::server->rank(),
        rpc::async(fmap_boundaries_client_local, f, xs, bss, as...));
  }

  static rpc::client<R>
  fmap_boundaries(const F &f, const rpc::client<T> &xs,
                  const cxx::boundaries<rpc::client<B>, D> &bss,
                  const As &... as) {
    return rpc::client<R>(
        rpc::async(fmap_boundaries_client, f, xs, bss, as...));
  }
};

template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
struct client_functor_boundaries<F, true, T, B, D, As...> {
  static_assert(rpc::is_action<F>::value, "");

  typedef cxx::invoke_of_t<F, T, cxx::boundaries<B, D>, As...> R;

  static rpc::global_shared_ptr<R>
  fmap_boundaries_client_remote(const F &f, const rpc::client<T> &xs,
                                const cxx::boundaries<rpc::client<B>, D> &bss,
                                const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(
        fmap_boundaries_client_remote);
    auto tmpbss =
        cxx::fmap([](const auto &bs) { return bs.make_local(); }, bss);
    auto newbss = cxx::fmap([](const auto &bs) { return *bs; }, tmpbss);
    return rpc::make_global_shared<R>(cxx::invoke(f, *xs, newbss, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_boundaries_client_remote);

  static rpc::client<R>
  fmap_boundaries_client(const F &f, const rpc::client<T> &xs,
                         const cxx::boundaries<rpc::client<B>, D> &bss,
                         const As &... as) {
    bool s = bool(xs);
    // assert(cxx::all_of(
    //     cxx::fmap([s](const auto &bs) { return bool(bs) == s; }, bss)));
    assert(cxx::foldMap([s](const auto &bs) { return bool(bs) == s; },
                        std::logical_and<bool>(), true, bss));
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(xs.get_proc(),
                          rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                     fmap_boundaries_client_remote_action(), f,
                                     xs, bss, as...));
  }

  static rpc::client<R>
  fmap_boundaries(const F &f, const rpc::client<T> &xs,
                  const cxx::boundaries<rpc::client<B>, D> &bss,
                  const As &... as) {
    return rpc::client<R>(
        rpc::async(fmap_boundaries_client, f, xs, bss, as...));
  }
};
// Define action exports
template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
typename client_functor_boundaries<
    F, true, T, B, D, As...>::fmap_boundaries_client_remote_evaluate_export_t
    client_functor_boundaries<
        F, true, T, B, D,
        As...>::fmap_boundaries_client_remote_evaluate_export =
        fmap_boundaries_client_remote_evaluate_export_init();
template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
typename client_functor_boundaries<
    F, true, T, B, D, As...>::fmap_boundaries_client_remote_finish_export_t
    client_functor_boundaries<
        F, true, T, B, D, As...>::fmap_boundaries_client_remote_finish_export =
        fmap_boundaries_client_remote_finish_export_init();

// template <typename F, typename T, typename... As>
// auto boundary(const F &f, const rpc::client<T> &xs, std::ptrdiff_t dir,
//               bool face, const As &... as) {
//   typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;
//   assert(bool(xs));
//   return fmap(f, xs, dir, face, as...);
// }

// foldable

template <typename T> const T &head(const rpc::client<T> &xs) {
  assert(bool(xs));
  return *xs;
}
template <typename T> const T &last(const rpc::client<T> &xs) {
  assert(bool(xs));
  return *xs;
}

template <typename Op, typename T, typename... As>
T fold(const Op &op, const T &z, const rpc::client<T> &xs, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, T, T, As...>, T>::value, "");
  bool s = bool(xs);
  if (!rpc::is_action<Op>::value) {
    // Note: We could call make_local, but we don't want to for non-actions
    return s == false ? z : *xs;
  }
  return s == false ? z : *xs.make_local();
}

template <typename Op, typename T, typename... As>
T fold1(const Op &op, const rpc::client<T> &xs, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, T, T, As...>, T>::value, "");
  bool s = bool(xs);
  assert(s);
  if (!rpc::is_action<Op>::value) {
    return *xs;
  }
  return *xs.make_local();
}

template <typename F, typename Op, typename R, typename T, typename... As>
R foldMap(const F &f, const Op &op, const R &z, const rpc::client<T> &xs,
          const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::fold(op, z, cxx::fmap(f, xs, as...));
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
R foldMap2(const F &f, const Op &op, const R &z, const rpc::client<T> &xs,
           const rpc::client<T2> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value, "");
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::fold(op, z, cxx::fmap2(f, xs, ys, as...));
}

template <typename F, typename Op, typename T, typename... As,
          typename R = cxx::invoke_of_t<F, T, As...> >
R fold1Map(const F &f, const Op &op, const rpc::client<T> &xs,
           const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::fold1(op, cxx::fmap(f, xs, as...));
}

template <typename F, typename Op, typename T, typename T2, typename... As,
          typename R = cxx::invoke_of_t<F, T, T2, As...> >
R fold1Map2(const F &f, const Op &op, const R &z, const rpc::client<T> &xs,
            const rpc::client<T2> &ys, const As &... as) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return cxx::fold1(op, cxx::fmap2(f, xs, ys, as...));
}

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus
// we cannot handle empty clients as monads.

template <template <typename> class C, typename T>
std::enable_if_t<cxx::is_client<C<T> >::value, C<T> > munit(const T &x) {
  // return rpc::make_client<T>(x);
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(), x);
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_client<C<T> >::value, C<T> > mmake(const As &... as) {
  // return rpc::make_client<T>(as...);
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(), as...);
}

namespace detail {
template <typename T, typename F, typename... As>
std::enable_if_t<rpc::is_action<F>::value, cxx::invoke_of_t<F, T, As...> >
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return cxx::invoke(f, *xs, as...);
}
template <typename T, typename F, typename... As>
struct bind_action
    : public rpc::action_impl<
          bind_action<T, F, As...>,
          rpc::wrap<decltype(&mbind<T, F, As...>), &mbind<T, F, As...> > > {};
// TODO: automate implementing this action:
// RPC_CLASS_EXPORT(rpc::detail::bind_action<T,  F, As...>::evaluate);
// RPC_CLASS_EXPORT(rpc::detail::bind_action<T,  F, As...>::finish);
}

template <typename T, typename F, typename... As, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
          typename R = typename cxx::kinds<CR>::value_type>
std::enable_if_t<!rpc::is_action<F>::value, C<R> >
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return rpc::client<R>(async([f](const rpc::client<T> &xs, const As &... as) {
                                return cxx::invoke(f, *xs, as...);
                              },
                              xs, as...));
}

template <typename T, typename F, typename... As, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
          typename R = typename cxx::kinds<CR>::value_type>
std::enable_if_t<rpc::is_action<F>::value, C<R> >
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return rpc::client<R>(async(rpc::rlaunch::async, xs.get_proc_future(),
                              detail::bind_action<T, F, As...>(), xs, F(),
                              as...));
}

// mapM :: Monad m => (a -> m b) -> [a] -> m [b]
// mapM_ :: Monad m => (a -> m b) -> [a] -> m ()
template <typename F, typename IT, typename... As,
          typename T = typename IT::value_type,
          typename CR = cxx::invoke_of_t<F, T, As...>,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R = typename cxx::kinds<CR>::value_type>
std::enable_if_t<cxx::is_client<CR>::value, C<std::tuple<> > >
mapM_(const F &f, const IT &xs, const As &... as) {
  for (const T &x : xs)
    cxx::invoke(f, x, as...);
  return munit<C>(std::tuple<>());
}

// TODO: execute remotely?
template <typename T, typename CCT = rpc::client<rpc::shared_future<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const rpc::client<rpc::client<T> > &xss) {
  return rpc::client<T>(rpc::async([xss]() { return *xss.make_local(); }));
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
