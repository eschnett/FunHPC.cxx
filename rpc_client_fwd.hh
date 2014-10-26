#ifndef RPC_CLIENT_FWD_HH
#define RPC_CLIENT_FWD_HH

#include "rpc_call_fwd.hh"
#include "rpc_defs.hh"
#include "rpc_future_fwd.hh"
#include "rpc_global_shared_ptr_fwd.hh"

#include <cereal/archives/binary.hpp>

#include <cassert>
#include <tuple>
#include <utility>

namespace rpc {

template <typename T> class client;

template <typename T> struct is_client : std::false_type {};
template <typename T> struct is_client<client<T> > : std::true_type {};

template <typename T, typename... As>
client<T> make_client(rpc::launch policy, const As &... args);

template <typename T, typename... As>
client<T> make_remote_client(rpc::rlaunch policy, int proc, const As &... args);
template <typename T, typename... As>
client<T> make_remote_client(rpc::rlaunch policy, const shared_future<int> &,
                             const As &... args);

// template <typename F, typename... As>
// auto local(launch policy, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
//
// template <typename F, typename... As>
// auto remote(rlaunch policy, int proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto remote(rlaunch policy, const shared_future<int> &proc, const F &f,
//             As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;

// template <typename F, typename... As>
// auto rewrap_client(int proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto rewrap_client(const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;

template <typename T> class client {
  // gcc 4.7 thinks that shared_future::get is non-const
  mutable rpc::shared_future<int> proc;
  mutable rpc::shared_future<global_shared_ptr<T> > data;

  bool proc_invariant() const {
    if (!future_is_ready(data))
      return true;
    if (!future_is_ready(proc))
      proc.wait();
    return proc.get() == data.get().get_proc();
  }

  template <typename U, typename... As>
  friend client<U> make_client(rpc::launch policy, const As &... args);

  template <typename U, typename... As>
  friend client<U> make_remote_client(rpc::rlaunch policy, int proc,
                                      const As &... args);
  template <typename U, typename... As>
  friend client<U> make_remote_client(rpc::rlaunch policy,
                                      const shared_future<int> &,
                                      const As &... args);

  // template <typename F, typename... As>
  // friend auto local(launch policy, const F &f, As &&... args)
  //     -> typename std::enable_if<
  //         is_client<typename cxx::invoke_of<F, As...>::type>::value,
  //         typename cxx::invoke_of<F, As...>::type>::type;
  //
  // template <typename F, typename... As>
  // friend auto remote(rlaunch policy, int proc, const F &f, As &&... args)
  //     -> typename std::enable_if<
  //         is_client<typename cxx::invoke_of<F, As...>::type>::value,
  //         typename cxx::invoke_of<F, As...>::type>::type;
  // template <typename F, typename... As>
  // friend auto remote(rlaunch policy, const shared_future<int> &proc, const F
  // &f,
  //                    As &&... args)
  //     -> typename std::enable_if<
  //         is_client<typename cxx::invoke_of<F, As...>::type>::value,
  //         typename cxx::invoke_of<F, As...>::type>::type;

  // template <typename F, typename... As>
  // friend auto rewrap_client(int proc, const F &f, As &&... args)
  //     -> typename std::enable_if<
  //         is_client<typename cxx::invoke_of<F, As...>::type>::value,
  //         typename cxx::invoke_of<F, As...>::type>::type;
  // template <typename F, typename... As>
  // friend auto rewrap_client(const F &f, As &&... args)
  //     -> typename std::enable_if<
  //         is_client<typename cxx::invoke_of<F, As...>::type>::value,
  //         typename cxx::invoke_of<F, As...>::type>::type;

public:
  typedef T element_type;
  typedef T value_type;

  // TODO: define then, unwrap for future<client> etc.?

  // We require explicit conversions for constructors that take
  // ownership of pointers
  client() : client(global_shared_ptr<T>()) {}
  client(const std::shared_ptr<T> &ptr) : client(global_shared_ptr<T>(ptr)) {}
  client(const global_shared_ptr<T> &ptr)
      : proc(make_ready_future(ptr.get_proc())), data(make_ready_future(ptr)) {}

  client(int proc, const rpc::shared_future<global_shared_ptr<T> > &ptr)
      : client(make_ready_future(proc), ptr) {
    assert(proc >= 0); // this could be omitted
  }
  client(int proc, rpc::future<global_shared_ptr<T> > &&ptr)
      : client(proc, ptr.share()) {}

  explicit client(const rpc::shared_future<client<T> > &ptr)
      : client(future_then(ptr, [](const rpc::shared_future<client<T> > &ptr)
                                    -> int { return ptr.get().get_proc(); }),
               future_then(ptr, [](const rpc::shared_future<client<T> > &ptr)
                                    -> global_shared_ptr<T> {
                 return ptr.get().data.get();
               })) {}
  explicit client(rpc::future<client<T> > &&ptr) : client(ptr.share()) {}

private:
  client(const rpc::shared_future<global_shared_ptr<T> > &ptr) : data(ptr) {
    proc =
        future_then(data, [](const shared_future<global_shared_ptr<T> > &data) {
          return data.get().get_proc();
        });
  }
  client(const rpc::shared_future<int> &proc,
         const rpc::shared_future<global_shared_ptr<T> > &ptr)
      : proc(proc), data(ptr) {}
  client(rpc::future<int> &&proc,
         const rpc::shared_future<global_shared_ptr<T> > &ptr)
      : client(proc.share(), ptr) {}

  client(rpc::future<global_shared_ptr<T> > &&ptr) : client(ptr.share()) {}
  client(const rpc::shared_future<int> &proc,
         rpc::future<global_shared_ptr<T> > &&ptr)
      : client(proc, ptr.share()) {}
  client(rpc::future<int> &&proc, rpc::future<global_shared_ptr<T> > &&ptr)
      : client(std::move(proc), ptr.share()) {}

  // client(int proc, const rpc::shared_future<std::shared_ptr<T> > &ptr)
  //     : client(proc,
  //              future_then(
  //                  ptr, [](const rpc::shared_future<std::shared_ptr<T> >
  //                  &ptr)
  //                           -> global_shared_ptr<T> { return ptr.get(); }))
  //                           {}
  // client(int proc, future<std::shared_ptr<T> > &&ptr)
  //     : client(proc, std::move(ptr).share()) {}

  client(int proc, const rpc::shared_future<client<T> > &ptr)
      : client(proc,
               future_then(ptr, [](const rpc::shared_future<client<T> > &ptr)
                                    -> global_shared_ptr<T> {
                 return ptr.get().data.get();
               })) {
    assert(proc >= 0);
  }
  client(const rpc::shared_future<int> &proc,
         const rpc::shared_future<client<T> > &ptr)
      : client(proc,
               future_then(ptr, [](const rpc::shared_future<client<T> > &ptr)
                                    -> global_shared_ptr<T> {
                 return ptr.get().data.get();
               })) {}
  client(rpc::future<int> &&proc, const rpc::shared_future<client<T> > &ptr)
      : client(proc.share(), ptr) {}

  client(int proc, rpc::future<client<T> > &&ptr) : client(proc, ptr.share()) {}
  client(const rpc::shared_future<int> &proc, rpc::future<client<T> > &&ptr)
      : client(proc, ptr.share()) {}
  client(rpc::future<int> &&proc, rpc::future<client<T> > &&ptr)
      : client(proc, ptr.share()) {}

public:
  global_shared_ptr<T> get_global_shared() const { return data.get(); }

#if 0
  operator bool() const { return bool(data.get()); }
  int get_proc() const { return data.get().get_proc(); }
  bool proc_is_ready() const { return future_is_ready(data); }
  shared_future<int> get_proc_future() const {
    return future_then(data,
                       [](const shared_future<global_shared_ptr<T> > &data) {
      return data.get().get_proc();
    });
  }
  bool is_local() const { return data.get().is_local(); }
#else
  operator bool() const { return proc.get() >= 0; }
  int get_proc() const {
    assert(proc_invariant());
    return proc.get();
  }
  bool proc_is_ready() const { return future_is_ready(proc); }
  shared_future<int> get_proc_future() const { return proc; }
  bool is_local() const {
    int p = proc.get();
    return p == -1 || p == server->rank();
  }
#endif

  bool operator==(const client &other) const {
    return data.get() == other.data.get();
  }
  bool operator!=(const client &other) const { return !(*this == other); }

  std::tuple<> wait() const {
    std::cout << "cw.0\n";
    proc.wait();
    std::cout << "cw.1\n";
    data.wait();
    std::cout << "cw.2\n";
    return {};
  }
  const std::shared_ptr<T> &get() const {
    assert(proc_invariant());
    return data.get().get();
  }
  T &operator*() const { return *get(); }
  auto operator -> () const -> decltype(this -> get()) { return get(); }

  client make_local() const {
    return client(future_then(
        data,
        [](const rpc::shared_future<global_shared_ptr<T> > &data)
            -> global_shared_ptr<T> { return data.get().make_local().get(); }));
  }

  // template <typename U = T>
  // auto unwrap() const -> typename std::enable_if<is_client<U>::value,
  // U>::type {
  //   return U(-2, async([=]() { return *make_local().get(); }));
  // }

  std::ostream &output(std::ostream &os) const { return os << data.get(); }

private:
  friend class cereal::access;
  template <class Archive> void serialize(Archive &ar);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const client<T> &ptr) {
  return ptr.output(os);
}

// synchronous
template <typename T, typename... As>
client<T> make_client(const As &... args) {
  return make_global_shared<T>(args...);
}

// asynchronous
template <typename T, typename... As>
client<T> make_client(rpc::launch policy, const As &... args) {
  // return rpc::async(policy, make_global_shared<T>, args...);
  return client<T>(rpc::server->rank(), rpc::async(policy, [=]() {
                                          return make_global_shared<T>(args...);
                                        }));
}

template <typename T, typename... As>
client<T> make_remote_client(int proc, const As &... args);
template <typename T, typename... As>
client<T> make_remote_client(const shared_future<int> &proc,
                             const As &... args);
template <typename T, typename... As>
client<T> make_remote_client(rpc::rlaunch policy, int proc, const As &... args);
template <typename T, typename... As>
client<T> make_remote_client(rpc::rlaunch policy, const shared_future<int> &,
                             const As &... args);

// template <typename F, typename... As>
// auto local(launch policy, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto local(const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
//
// template <typename F, typename... As>
// auto remote(rlaunch policy, int proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto remote(rlaunch policy, const shared_future<int> &proc, const F &f,
//             As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto remote(int proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto remote(const shared_future<int> &proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename G, typename... As>
// auto remote(rlaunch policy, F, G &&global, As &&... args)
//     -> typename std::enable_if<
//         is_action<F>::value && is_global<G>::value &&
//             is_client<typename cxx::invoke_of<F, G, As...>::type>::value,
//         typename cxx::invoke_of<F, G, As...>::type>::type;
// template <typename F, typename G, typename... As>
// auto remote(F, G &&global, As &&... args)
//     -> typename std::enable_if<
//         is_action<F>::value && is_global<G>::value &&
//             is_client<typename cxx::invoke_of<F, G, As...>::type>::value,
//         typename cxx::invoke_of<F, G, As...>::type>::type;

// template <typename F, typename... As>
// auto rewrap_client(int proc, const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
// template <typename F, typename... As>
// auto rewrap_client(const F &f, As &&... args)
//     -> typename std::enable_if<
//         is_client<typename cxx::invoke_of<F, As...>::type>::value,
//         typename cxx::invoke_of<F, As...>::type>::type;
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#ifndef RPC_CLIENT_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_FWD_HH
