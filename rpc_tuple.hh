#ifndef RPC_TUPLE_HH
#define RPC_TUPLE_HH

#include <boost/serialization/access.hpp>

#include <iostream>
#include <tuple>
#include <type_traits>

namespace rpc {
  
  using std::enable_if;
  using std::get;
  using std::istream;
  using std::ostream;
  using std::result_of;
  using std::tuple;
  
  
  
  // template<size_t N>
  // struct tuple_bind_impl {
  //   template<typename... As>
  //   bind ...
  // };
  
  template<typename F, typename... As>
  struct tuple_map_impl {
    const F& f;
    const tuple<As...>& t;
    typedef typename result_of<F(As...)>::type R;
    template<typename... As1>
    auto map(const As1&... args1) const ->
      typename enable_if<sizeof...(As1) < sizeof...(As), R>::type
    {
      return map(args1..., get<sizeof...(As1)>(t));
    }
    template<typename... As1>
    auto map(const As1&... args1) const ->
      typename enable_if<sizeof...(As1) == sizeof...(As), R>::type
    {
      return f(args1...);
    }
  };
  
  template<typename F, typename... As>
  auto tuple_map(const F& f, const tuple<As...>& t) ->
    typename result_of<F(As...)>::type
  {
    return tuple_map_impl<F, As...>({ f, t }).map();
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

#define RPC_TUPLE_HH_DONE
#else
#  ifndef RPC_TUPLE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_TUPLE_HH
