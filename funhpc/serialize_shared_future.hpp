// -*-C++-*-
#ifndef SERIALIZE_SHARED_FUTURE_HPP
#define SERIALIZE_SHARED_FUTURE_HPP

#include <funhpc/async.hpp>
#include <funhpc/rptr.hpp>
#include <qthread/future.hpp>

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>

namespace qthread {

namespace detail {
template <typename T>
T shared_future_get(const funhpc::rptr<shared_future<T>> &pf) {
  T val = pf->get();
  delete pf.get_ptr();
  return val;
}
}

template <typename Archive, typename T>
void save(Archive &ar, const shared_future<T> &f) {
  bool v = f.valid();
  ar(v);
  if (v) {
    bool r = f.ready();
    ar(r);
    if (r) {
      ar(f.get());
    } else {
      funhpc::rptr<shared_future<T>> pf(new shared_future<T>(f));
      ar(pf);
    }
  }
}

template <typename Archive, typename T>
void load(Archive &ar, shared_future<T> &f) {
  bool v;
  ar(v);
  if (v) {
    bool r;
    ar(r);
    if (r) {
      T val;
      ar(val);
      f = make_ready_future(std::move(val));
    } else {
      funhpc::rptr<shared_future<T>> pf;
      ar(pf);
      f = funhpc::async(funhpc::rlaunch::async, pf.get_proc(),
                        detail::shared_future_get<T>, pf);
    }
  }
}
}

#define SERIALIZE_SHARED_FUTURE_HPP_DONE
#endif // #ifndef SERIALIZE_SHARED_FUTURE_HPP
#ifndef SERIALIZE_SHARED_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
