// -*-C++-*-
#ifndef SERIALIZE_SHARED_FUTURE_HPP
#define SERIALIZE_SHARED_FUTURE_HPP

#include <qthread/future.hpp>

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>

namespace qthread {
template <typename Archive, typename T>
void save(Archive &ar, const shared_future<T> &f) {
  ar(f.get());
}
template <typename Archive, typename T>
void load(Archive &ar, shared_future<T> &f) {
  T val;
  ar(val);
  f = make_ready_future(std::move(val));
}
}

#define SERIALIZE_SHARED_FUTURE_HPP_DONE
#endif // #ifndef SERIALIZE_SHARED_FUTURE_HPP
#ifndef SERIALIZE_SHARED_FUTURE_HPP_DONE
#error "Cyclic include dependency"
#endif
