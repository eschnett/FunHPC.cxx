#ifndef RPC_CLIENT_HH
#define RPC_CLIENT_HH

#include "rpc_client_fwd.hh"

#include "rpc_call.hh"
#include "rpc_global_shared_ptr.hh"

#include <boost/serialization/export.hpp>

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

namespace boost {
namespace serialization {

#ifdef HPX_DISABLE_FUTURE_SERIALIZE

// These don't work with HPX, since HPX's futures can already be
// serialized
template <typename Archive, typename T>
inline void save(Archive &ar, const rpc::shared_future<T> &data,
                 const unsigned int file_version) {
  bool ready = rpc::future_is_ready(data);
  ar << ready;
  if (ready) {
    // Send the data of the future
    ar << data.get();
  } else {
    // Create a global pointer to the future, and send it
    auto ptr = rpc::make_global<rpc::shared_future<T> >(data);
    ar << ptr;
  }
}

template <typename Archive, typename T>
inline void load(Archive &ar, rpc::shared_future<T> &data,
                 const unsigned int file_version) {
  // RPC_ASSERT(!data.valid());
  bool ready;
  ar >> ready;
  if (ready) {
    // Receive the data, and create a future from it
    T data_;
    ar >> data_;
    data = rpc::make_ready_future(std::move(data_));
  } else {
    // Create a future that is waiting for the data of the remote
    // future
    rpc::global_ptr<rpc::shared_future<T> > ptr;
    ar >> ptr;
    data = async(rpc::remote::async, ptr.get_proc(),
                 rpc::shared_future_get_action<T>(), ptr);
  }
}

template <typename Archive, typename T>
inline void serialize(Archive &ar, rpc::shared_future<T> &data,
                      const unsigned int file_version) {
  split_free(ar, data, file_version);
}

#else

typedef boost::mpi::packed_oarchive OArchive;
template <typename T>
inline void save(OArchive &ar, const rpc::shared_future<T> &data,
                 const unsigned int file_version) {
  bool ready = rpc::future_is_ready(data);
  ar << ready;
  if (ready) {
    // Send the data of the future
    ar << data.get();
  } else {
    // Create a global pointer to the future, and send it
    auto ptr = rpc::make_global<rpc::shared_future<T> >(data);
    ar << ptr;
  }
}
template <typename T>
inline void serialize(OArchive &ar, rpc::shared_future<T> &data,
                      const unsigned int file_version) {
  split_free(ar, data, file_version);
}

typedef boost::mpi::packed_iarchive IArchive;
template <typename T>
inline void load(IArchive &ar, rpc::shared_future<T> &data,
                 const unsigned int file_version) {
  // RPC_ASSERT(!data.valid());
  bool ready;
  ar >> ready;
  if (ready) {
    // Receive the data, and create a future from it
    T data_;
    ar >> data_;
    data = rpc::make_ready_future(std::move(data_));
  } else {
    // Create a future that is waiting for the data of the remote
    // future
    rpc::global_ptr<rpc::shared_future<T> > ptr;
    ar >> ptr;
    data = async(rpc::remote::async, ptr.get_proc(),
                 rpc::shared_future_get_action<T>(), ptr);
  }
}
template <typename T>
inline void serialize(IArchive &ar, rpc::shared_future<T> &data,
                      const unsigned int file_version) {
  split_free(ar, data, file_version);
}

#endif
}
}

namespace rpc {

template <typename T>
template <class Archive>
void client<T>::serialize(Archive &ar, unsigned int version) {
  ar &data;
}

template <typename T, typename... As>
client<T> make_remote_client(int proc, const As &... args) {
  return make_remote_client<T>(remote::async, proc, args...);
}

template <typename T, typename... As>
client<T> make_remote_client(remote policy, int proc, const As &... args) {
  return async(policy, proc, make_global_shared_action<T, As...>(), args...);
}
}

#define RPC_CLIENT_HH_DONE
#else
#ifndef RPC_CLIENT_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_CLIENT_HH
