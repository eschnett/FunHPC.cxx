#ifndef RPC_DEFS_HH
#define RPC_DEFS_HH

#include <boost/mpi.hpp>

#include <tuple>

namespace rpc {
  
  using namespace boost;
  
  using std::enable_if;
  using std::tuple;
  using std::tuple_size;
  
  
  
  // TODO: Add more definitions, maybe using recursion
  
  template<typename F>
  auto tuple_map(const F& func, const tuple<>& t) ->
    decltype(func())
  {
    return func();
  }
  
  template<typename F, typename A0>
  auto tuple_map(const F& func, const tuple<A0>& t) ->
    decltype(func(get<0>(t)))
  {
    return func(get<0>(t));
  }
  
  template<typename F, typename A0, typename A1>
  auto tuple_map(const F& func, const tuple<A0, A1>& t) ->
    decltype(func(get<0>(t), get<1>(t)))
  {
    return func(get<0>(t), get<1>(t));
  }
  
  template<typename F, typename A0, typename A1, typename A2>
  auto tuple_map(const F& func, const tuple<A0, A1, A2>& t) ->
    decltype(func(get<0>(t), get<1>(t), get<2>(t)))
  {
    return func(get<0>(t), get<1>(t), get<2>(t));
  }
  
  template<typename F, typename A0, typename A1, typename A2, typename A3>
  auto tuple_map(const F& func, const tuple<A0, A1, A2, A3>& t) ->
    decltype(func(get<0>(t), get<1>(t), get<2>(t), get<3>(t)))
  {
    return func(get<0>(t), get<1>(t), get<2>(t), get<3>(t));
  }
  
  
  
  typedef int proc_t;
  extern mpi::communicator comm;
  
}

#endif  // #ifndef RPC_DEFS_HH
