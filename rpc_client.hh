#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

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
    // TODO: We don't like the call to make_Local here, but iterators
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

// foldable

// TODO: Allow additional arguments for fold?
// TODO: Implement fold in terms of foldMap?
template <typename Op, bool is_action, typename R> struct client_fold;

template <typename Op, typename R>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value, R>::type
fold(const Op &op, R z, const rpc::client<R> &xs) {
  return client_fold<Op, rpc::is_action<Op>::value, R>::fold(op, z, xs);
}

template <typename Op, typename R> struct client_fold<Op, false, R> {
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R fold_client(const Op &op, const R &z, const rpc::client<R> &xs) {
    return *xs;
  }

  static R fold(const Op &op, const R &z, const rpc::client<R> &xs) {
    bool s = bool(xs);
    return s == false ? z : fold_client(op, z, xs);
  }
};

template <typename Op, typename R> struct client_fold<Op, true, R> {
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R fold_client(const R &z, const rpc::client<R> &xs) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fold_client);
    return *xs;
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fold_client);

  static R fold(Op, const R &z, const rpc::client<R> &xs) {
    bool s = bool(xs);
    return s == false ? z : rpc::sync(rpc::rlaunch::sync, xs.get_proc(),
                                      fold_client_action(), z, xs);
  }
};
// Define action exports
template <typename Op, typename R>
typename client_fold<Op, true, R>::fold_client_evaluate_export_t
    client_fold<Op, true, R>::fold_client_evaluate_export =
        fold_client_evaluate_export_init();
template <typename Op, typename R>
typename client_fold<Op, true, R>::fold_client_finish_export_t
    client_fold<Op, true, R>::fold_client_finish_export =
        fold_client_finish_export_init();

template <typename F, typename Op, bool is_action, typename R, typename T,
          typename... As>
struct client_foldable;

template <typename T> const T &head(const rpc::client<T> &xs) {
  assert(bool(xs));
  return *xs;
}
template <typename T> const T &last(const rpc::client<T> &xs) {
  assert(bool(xs));
  return *xs;
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, R z, const rpc::client<T> &xs,
        const As &... as) {
  return client_foldable<F, Op, rpc::is_action<Op>::value, R, T,
                         As...>::foldMap(f, op, z, xs, as...);
}

template <typename F, typename Op, typename R, typename T, typename... As>
struct client_foldable<F, Op, false, R, T, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R foldMap_client(const F &f, const Op &op, const R &z,
                          const rpc::client<T> &xs, const As &... as) {
    return cxx::invoke(f, *xs, as...);
  }

  static R foldMap(const F &f, const Op &op, const R &z,
                   const rpc::client<T> &xs, const As &... as) {
    bool s = bool(xs);
    return s == false ? z : foldMap_client(f, op, z, xs, as...);
  }
};

template <typename F, typename Op, typename R, typename T, typename... As>
struct client_foldable<F, Op, true, R, T, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value, "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R foldMap_client(const R &z, const rpc::client<T> &xs,
                          const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_client);
    return cxx::invoke(F(), *xs, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap_client);

  static R foldMap(F, Op, const R &z, const rpc::client<T> &xs,
                   const As &... as) {
    bool s = bool(xs);
    return s == false ? z : rpc::sync(rpc::rlaunch::sync, xs.get_proc(),
                                      foldMap_client_action(), z, xs, as...);
  }
};
// Define action exports
template <typename F, typename Op, typename R, typename T, typename... As>
typename client_foldable<F, Op, true, R, T,
                         As...>::foldMap_client_evaluate_export_t
    client_foldable<F, Op, true, R, T, As...>::foldMap_client_evaluate_export =
        foldMap_client_evaluate_export_init();
template <typename F, typename Op, typename R, typename T, typename... As>
typename client_foldable<F, Op, true, R, T,
                         As...>::foldMap_client_finish_export_t
    client_foldable<F, Op, true, R, T, As...>::foldMap_client_finish_export =
        foldMap_client_finish_export_init();

template <typename F, typename Op, bool is_action, typename R, typename T,
          typename T2, typename... As>
struct client_foldable2;

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const Op &op, R z, const rpc::client<T> &xs,
         const rpc::client<T2> &ys, const As &... as) {
  return client_foldable2<F, Op, rpc::is_action<Op>::value, R, T, T2,
                          As...>::foldMap2(f, op, z, xs, ys, as...);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
struct client_foldable2<F, Op, false, R, T, T2, As...> {
  static_assert(!rpc::is_action<F>::value, "");
  static_assert(!rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value,
      "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R foldMap2_client(const F &f, const Op &op, const R &z,
                           const rpc::client<T> &xs, const rpc::client<T2> &ys,
                           const As &... as) {
    return cxx::invoke(f, *xs, *ys, as...);
  }

  static R foldMap2(const F &f, const Op &op, const R &z,
                    const rpc::client<T> &xs, const rpc::client<T2> &ys,
                    const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? z : foldMap2_client(f, op, z, xs, ys, as...);
  }
};

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
struct client_foldable2<F, Op, true, R, T, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(rpc::is_action<Op>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value,
      "");
  static_assert(std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value,
                "");

  static R foldMap2_client(const R &z, const rpc::client<T> &xs,
                           const rpc::client<T2> &ys, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap2_client);
    return cxx::invoke(F(), *xs, *ys, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldMap2_client);

  static R foldMap2(F, Op, const R &z, const rpc::client<T> &xs,
                    const rpc::client<T2> &ys, const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? z
                      : rpc::sync(rpc::rlaunch::sync, xs.get_proc(),
                                  foldMap2_client_action(), z, xs, ys, as...);
  }
};
// Define action exports
template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename client_foldable2<F, Op, true, R, T, T2,
                          As...>::foldMap2_client_evaluate_export_t
    client_foldable2<F, Op, true, R, T, T2,
                     As...>::foldMap2_client_evaluate_export =
        foldMap2_client_evaluate_export_init();
template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename client_foldable2<F, Op, true, R, T, T2,
                          As...>::foldMap2_client_finish_export_t
    client_foldable2<F, Op, true, R, T, T2,
                     As...>::foldMap2_client_finish_export =
        foldMap2_client_finish_export_init();

#if 0
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

  static R foldl_client(const F &f, const R &z, const rpc::client<T> &xs,
                        const As &... as) {
    return cxx::invoke(f, z, *xs, as...);
  }

  static R foldl(const F &f, const R &z, const rpc::client<T> &xs,
                 const As &... as) {
    bool s = bool(xs);
    return s == false ? z : foldl_client(f, z, xs, as...);
  }
};

template <typename F, typename R, typename T, typename... As>
struct client_foldable<F, true, R, T, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
      "");

  static R foldl_client(const R &z, const rpc::client<T> &xs,
                        const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_client);
    return cxx::invoke(F(), z, *xs, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl_client);

  static R foldl(F, const R &z, const rpc::client<T> &xs, const As &... as) {
    bool s = bool(xs);
    return s == false ? z : rpc::sync(rpc::rlaunch::sync, xs.get_proc(),
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
    return s == false ? z : foldl2_client(f, z, xs, ys, as...);
  }
};

template <typename F, typename R, typename T, typename T2, typename... As>
struct client_foldable2<F, true, R, T, T2, As...> {
  static_assert(rpc::is_action<F>::value, "");
  static_assert(
      std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
      "");

  static R foldl2_client(const R &z, const rpc::client<T> &xs,
                         const rpc::client<T2> &ys, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_client);
    auto ysl = ys.make_local();
    return cxx::invoke(F(), z, *xs, *ysl, as...);
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(foldl2_client);

  static R foldl2(F, const R &z, const rpc::client<T> &xs,
                  const rpc::client<T2> &ys, const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    return s == false ? z : rpc::sync(rpc::rlaunch::sync, xs.get_proc(),
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
#endif

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

  typedef typename cxx::invoke_of<F, T, As...>::type R;

  static rpc::global_shared_ptr<R> fmap_client_remote(const rpc::client<T> &xs,
                                                      const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client_remote);
    return rpc::make_global_shared<R>(cxx::invoke(F(), *xs, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_client_remote);

  static rpc::client<R> fmap_client(const rpc::client<T> &xs,
                                    const As &... as) {
    bool s = bool(xs);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(xs.get_proc(),
                          rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                     fmap_client_remote_action(), xs, as...));
  }

  static rpc::client<R> fmap(F, const rpc::client<T> &xs, const As &... as) {
    return rpc::client<R>(rpc::async(fmap_client, xs, as...));
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

  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;

  static rpc::global_shared_ptr<R> fmap2_client_remote(const rpc::client<T> &xs,
                                                       const rpc::client<T> &ys,
                                                       const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client_remote);
    auto ysl = ys.make_local();
    return rpc::make_global_shared<R>(cxx::invoke(F(), *xs, *ysl, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap2_client_remote);

  static rpc::client<R> fmap2_client(const rpc::client<T> &xs,
                                     const rpc::client<T2> &ys,
                                     const As &... as) {
    bool s = bool(xs);
    assert(bool(ys) == s);
    if (s == false)
      return rpc::client<R>();
    return rpc::client<R>(
        xs.get_proc(), rpc::async(rpc::rlaunch::async, xs.get_proc(),
                                  fmap2_client_remote_action(), xs, ys, as...));
  }

  static rpc::client<R> fmap2(F, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys, const As &... as) {
    return rpc::client<R>(rpc::async(fmap2_client, xs, ys, as...));
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

  typedef typename cxx::invoke_of<F, T, T2, T3, As...>::type R;

  static rpc::global_shared_ptr<R>
  fmap3_client_remote(const rpc::client<T> &xs, const rpc::client<T2> &ys,
                      const rpc::client<T3> &zs, const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client_remote);
    auto ysl = ys.make_local();
    auto zsl = zs.make_local();
    return rpc::make_global_shared<R>(cxx::invoke(F(), *xs, *ysl, *zsl, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap3_client_remote);

  static rpc::client<R> fmap3_client(const rpc::client<T> &xs,
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
                                     fmap3_client_remote_action(), xs, ys, zs,
                                     as...));
  }

  static rpc::client<R> fmap3(F, const rpc::client<T> &xs,
                              const rpc::client<T2> &ys,
                              const rpc::client<T3> &zs, const As &... as) {
    return rpc::client<R>(rpc::async(fmap3_client, xs, ys, zs, as...));
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
          typename R =
              typename cxx::invoke_of<F, T, cxx::boundaries<B, D>, As...>::type>
rpc::client<R> fmap_boundaries(const F &f, const rpc::client<T> &xs,
                               const cxx::boundaries<rpc::client<B>, D> &bss,
                               const As &... as) {
  return client_functor_boundaries<F, rpc::is_action<F>::value, T, B, D,
                                   As...>::fmap_boundaries(f, xs, bss, as...);
}

template <typename F, typename T, typename B, std::ptrdiff_t D, typename... As>
struct client_functor_boundaries<F, false, T, B, D, As...> {
  static_assert(!rpc::is_action<F>::value, "");

  typedef typename cxx::invoke_of<F, T, cxx::boundaries<B, D>, As...>::type R;

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

  typedef typename cxx::invoke_of<F, T, cxx::boundaries<B, D>, As...>::type R;

  static rpc::global_shared_ptr<R>
  fmap_boundaries_client_remote(const rpc::client<T> &xs,
                                const cxx::boundaries<rpc::client<B>, D> &bss,
                                const As &... as) {
    RPC_INSTANTIATE_TEMPLATE_STATIC_MEMBER_ACTION(
        fmap_boundaries_client_remote);
    auto tmpbss =
        cxx::fmap([](const auto &bs) { return bs.make_local(); }, bss);
    auto newbss = cxx::fmap([](const auto &bs) { return *bs; }, tmpbss);
    return rpc::make_global_shared<R>(cxx::invoke(F(), *xs, newbss, as...));
  }
  RPC_DECLARE_TEMPLATE_STATIC_MEMBER_ACTION(fmap_boundaries_client_remote);

  static rpc::client<R>
  fmap_boundaries_client(const rpc::client<T> &xs,
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
                                     fmap_boundaries_client_remote_action(), xs,
                                     bss, as...));
  }

  static rpc::client<R>
  fmap_boundaries(F, const rpc::client<T> &xs,
                  const cxx::boundaries<rpc::client<B>, D> &bss,
                  const As &... as) {
    return rpc::client<R>(rpc::async(fmap_boundaries_client, xs, bss, as...));
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

// monad

// Note: We cannot unwrap a future where the inner future is invalid. Thus
// we cannot handle empty clients as monads.

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  // return rpc::make_client<T>(std::forward<T1>(x));
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(),
                                    std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  // return rpc::make_client<T>(std::forward<As>(as)...);
  return rpc::make_remote_client<T>(rpc::detail::choose_dest(),
                                    std::forward<As>(as)...);
}

namespace detail {
template <typename T, typename F, typename... As>
typename std::enable_if<rpc::is_action<F>::value,
                        typename cxx::invoke_of<F, T, As...>::type>::type
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return cxx::invoke(F(), *xs, as...);
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
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<!rpc::is_action<F>::value, C<R> >::type
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return rpc::client<R>(async([f](const rpc::client<T> &xs, const As &... as) {
                                return cxx::invoke(f, *xs, as...);
                              },
                              xs, as...));
}

template <typename T, typename F, typename... As, typename CT = rpc::client<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<rpc::is_action<F>::value, C<R> >::type
mbind(const rpc::client<T> &xs, const F &f, const As &... as) {
  return rpc::client<R>(async(rpc::rlaunch::async, xs.get_proc_future(),
                              detail::bind_action<T, F, As...>(), xs, F(),
                              as...));
}

// mapM :: Monad m => (a -> m b) -> [a] -> m [b]
// mapM_ :: Monad m => (a -> m b) -> [a] -> m ()
template <typename F, typename IT, typename... As,
          typename T = typename IT::value_type,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_client<CR>::value, C<std::tuple<> > >::type
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

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = typename cxx::invoke_of<F, std::ptrdiff_t, As...>::type>
typename std::enable_if<cxx::is_client<C<T> >::value, C<T> >::type
iota(const F &f, const iota_range_t &range, const As &... as) {
  assert(range.local.size() == 1);
  // return munit<C>(cxx::invoke(f, range.local.imin, as...));
  int dest =
      div_floor(rpc::server->size() * (range.local.imin - range.global.imin),
                range.global.imax - range.global.imin);
  return rpc::make_remote_client<T>(dest,
                                    cxx::invoke(f, range.local.imin, as...));
}

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As,
          typename T = typename cxx::invoke_of<F, cxx::grid_region<D>,
                                               cxx::index<D>, As...>::type,
          std::enable_if_t<cxx::is_client<C<T> >::value> * = nullptr>
auto iota(const F &f, const cxx::grid_region<D> &global_range,
          const cxx::grid_region<D> &range, const As &... as) {
  assert(range.size() == 1);
  // return munit<C>(cxx::invoke(f, range.local.imin, as...));
  ptrdiff_t idx = global_range.linear(range.imin());
  ptrdiff_t idx_max = global_range.linear_max();
  int dest = div_floor(rpc::server->size() * idx, idx_max);
  return rpc::make_remote_client<T>(
      dest, cxx::invoke(f, global_range, range.imin(), as...));
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
