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

template <typename T> class client {
  // gcc 4.7 thinks that shared_future::get is non-const
  mutable rpc::shared_future<global_shared_ptr<T> > data;

public:
  typedef T element_type;
  typedef T value_type;

  // TODO: define then, unwrap for future<client> etc.?

  // We require explicit conversions for constructors that take
  // ownership of pointers
  client() : client(global_shared_ptr<T>()) {}
  client(const std::shared_ptr<T> &ptr) : client(global_shared_ptr<T>(ptr)) {}
  client(const global_shared_ptr<T> &ptr) : data(make_ready_future(ptr)) {}
  client(const rpc::shared_future<global_shared_ptr<T> > &ptr) : data(ptr) {}
  client(future<global_shared_ptr<T> > &&ptr) : client(ptr.share()) {}

  client(const rpc::shared_future<std::shared_ptr<T> > &ptr)
      : client(future_then(
            ptr, [](const rpc::shared_future<std::shared_ptr<T> > &ptr)
                     -> global_shared_ptr<T> { return ptr.get(); })) {}
  client(future<std::shared_ptr<T> > &&ptr) : client(ptr.share()) {}

  client(const rpc::shared_future<client<T> > &ptr)
      : client(future_then(ptr, [](const rpc::shared_future<client<T> > &ptr)
                                    -> global_shared_ptr<T> {
          return ptr.get().data.get();
        })) {}
  client(future<client<T> > &&ptr) : client(ptr.share()) {}

  global_shared_ptr<T> get_global_shared() const { return data.get(); }

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

  bool operator==(const client &other) const {
    return data.get() == other.data.get();
  }
  bool operator!=(const client &other) const { return !(*this == other); }

  std::tuple<> wait() const {
    data.wait();
    return {};
  }
  const std::shared_ptr<T> &get() const { return data.get().get(); }
  T &operator*() const { return *get(); }
  auto operator -> () const -> decltype(this -> get()) { return get(); }

  client make_local() const {
    return future_then(
        data,
        [](const rpc::shared_future<global_shared_ptr<T> > &data)
            -> global_shared_ptr<T> { return data.get().make_local().get(); });
  }

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
  return rpc::async(policy, [=]() { return make_global_shared<T>(args...); });
}

template <typename T, typename... As>
client<T> make_remote_client(int proc, const As &... args);

template <typename T, typename... As>
client<T> make_remote_client(const shared_future<int> &proc,
                             const As &... args);

template <typename T, typename... As>
client<T> make_remote_client(rpc::remote policy, int proc, const As &... args);

template <typename T, typename... As>
client<T> make_remote_client(rpc::remote policy, const shared_future<int> &,
                             const As &... args);
}

#define RPC_CLIENT_FWD_HH_DONE
#else
#ifndef RPC_CLIENT_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_FWD_HH
