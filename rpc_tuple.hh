#ifndef RPC_TUPLE_HH
#define RPC_TUPLE_HH

#include <boost/serialization/access.hpp>

#include <iostream>
#include <tuple>

namespace rpc {
  
  using std::get;
  using std::istream;
  using std::ostream;
  using std::tuple;
  
  
  
  // TODO: Add more definitions, maybe using recursion
  
  // template<size_t N>
  // struct tuple_bind_impl {
  //   template<typename... As>
  //   bind ...
  // };
  
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
  
  
  
  template<size_t N>
  struct input_tuple_impl {
    template<typename... As>
    static void input(istream& is, tuple<As...>& t)
    {
      input_tuple_impl<N-1>::input(is, t);
      is >> get<N-1>(t);
    }
  };
  
  template<>
  struct input_tuple_impl<0> {
    template<typename... As>
    static void input(istream& is, tuple<As...>& t)
    {
    }
  };
  
  
  
  template<size_t N>
  struct output_tuple_impl {
    template<typename... As>
    static void output(ostream& os, const tuple<As...>& t)
    {
      output_tuple_impl<N-1>::output(os, t);
      if (N>1) os << ",";
      os << get<N-1>(t);
    }
  };
  
  template<>
  struct output_tuple_impl<0> {
    template<typename... As>
    static void output(ostream& os, const tuple<As...>& t)
    {
    }
  };
  
  
  
  // Inspired by Sydius at
  // <https://sydius.me/2011/02/c0x-tuple-boost-serialization/>
  template<size_t N>
  struct serialize_tuple_impl {
    template<class Archive, typename... As>
    static void serialize(Archive& ar, tuple<As...>& t, unsigned int version)
    {
      serialize_tuple_impl<N-1>::serialize(ar, t, version);
      ar & get<N-1>(t);
    }
  };
  
  template<>
  struct serialize_tuple_impl<0> {
    template<class Archive, typename... As>
    static void serialize(Archive& ar, tuple<As...>& t, unsigned int version)
    {
    }
  };
  
}



namespace std {
  
  template<typename... As>
  istream& operator>>(istream& is, std::tuple<As...>& t)
  {
    rpc::input_tuple_impl<sizeof...(As)>::input(is, t);
    return is;
  }
  
  template<typename... As>
  ostream& operator<<(ostream& os, const std::tuple<As...>& t)
  {
    os << "(";
    rpc::output_tuple_impl<sizeof...(As)>::output(os, t);
    os << ")";
    return os;
  }
  
}

namespace boost {
  namespace serialization {
    
    template<class Archive, typename... As>
    void serialize(Archive & ar, std::tuple<As...>& t, unsigned int version)
    {
      rpc::serialize_tuple_impl<sizeof...(As)>::serialize(ar, t, version);
    }
    
  }
}

#endif  // #ifndef RPC_TUPLE_HH
