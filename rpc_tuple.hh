#ifndef RPC_TUPLE_HH
#define RPC_TUPLE_HH

#include "cxx_utils.hh"

#include <boost/serialization/access.hpp>

#include <iostream>
#include <tuple>
#include <type_traits>

namespace rpc {
  
  using std::enable_if;
  using std::forward;
  using std::get;
  using std::istream;
  using std::make_tuple;
  using std::ostream;
  using std::tuple;
  using std::tuple_cat;
  using std::tuple_element;
  
  
  
  // template<size_t N>
  // struct tuple_bind_impl {
  //   template<typename... As>
  //   bind ...
  // };
  
  // TODO: Make the function the first argument of the tuple?
  
  // template<typename F, typename... As>
  // struct tuple_apply0_impl {
  //   const F& f;
  //   const tuple<As...>& t;
  //   typedef typename invoke_of<F, As...>::type R;
  //   tuple_apply0_impl(const F& f, const tuple<As...>& t): f(f), t(t) {}
  //   template<typename... As1>
  //   auto apply(As1&&... args1) const ->
  //     typename enable_if<sizeof...(As1) < sizeof...(As), R>::type
  //   {
  //     return apply(forward<As1>(args1)..., get<sizeof...(As1)>(t));
  //   }
  //   template<typename... As1>
  //   auto apply(As1&&... args1) const ->
  //     typename enable_if<sizeof...(As1) == sizeof...(As), R>::type
  //   {
  //     return invoke(f, forward<As1>(args1)...);
  //   }
  // };
  
  // template<typename F, typename... As>
  // auto tuple_apply0(const F& f, const tuple<As...>& t) ->
  //   typename invoke_of<F, As...>::type
  // {
  //   return tuple_apply0_impl<F, As...>(f, t).apply();
  // }
  
  
  
  // template<size_t N>
  // struct tuple_index {
  //   static constexpr
  //   decltype(tuple_cat(tuple_index<N-1>::value, tuple<size_t>(N-1)))
  //     value = tuple_cat(tuple_index<N-1>::value, tuple<size_t>(N-1));
  // };
  
  // template<>
  // struct tuple_index<0> {
  //   static constexpr tuple<> value = tuple<>();
  // };
  
  
  
  namespace detail {
    template<size_t...>
    struct seq {};
    
    template<size_t N, size_t... S>
    struct make_seq: make_seq<N-1, N-1, S...> {};
    template<size_t... S>
    struct make_seq<0, S...> {
      typedef seq<S...> type;
    };
  }
  
  
  
  template<typename F, typename... As, size_t... S>
  auto tuple_apply_impl(const F& f, const tuple<As...>& t, detail::seq<S...>) ->
    typename invoke_of<F, As...>::type
  {
    return invoke(f, get<S>(t)...);
  }
  
  template<typename F, typename... As>
  auto tuple_apply(const F& f, const tuple<As...>& t) ->
    typename invoke_of<F, As...>::type
  {
    typename detail::make_seq<sizeof...(As)>::type s;
    return tuple_apply_impl(f, t, s);
  }
  
  
  
  template<template<typename> class F, typename... As, size_t... S>
  auto tuple_map_impl(const tuple<As...>& t, detail::seq<S...>) ->
    tuple<invoke_of<F<As>(As)>...>
  {
    return make_tuple(F<As>()(get<S>(t))...);
  }
  
  template<template<typename> class F, typename... As>
  auto tuple_map(const tuple<As...>& t) ->
    tuple<invoke_of<F<As>(As)>...>
  {
    return tuple_map_impl<F>(t, detail::make_seq<sizeof...(As)>::type());
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

#if defined RPC_HPX
#  include <hpx/util/serialize_empty_type.hpp>
#endif

namespace boost {
  namespace serialization {
    
#ifndef RPC_HPX
    
    template<class Archive, typename... As>
    void serialize(Archive & ar, std::tuple<As...>& t, unsigned int version)
    {
      rpc::serialize_tuple_impl<sizeof...(As)>::serialize(ar, t, version);
    }
    
#else
    
    // HPX provides a serialization function for its own archives and
    // empty types. This conflicts with tuples when the tuple is
    // empty. We thus need to provide tuple serialization functions
    // only for either non-HPX archives or non-empty tuples.
    
    template<class Archive, typename... As>
    typename std::enable_if<
      (!std::is_same<Archive, hpx::util::portable_binary_iarchive>::value &&
       !std::is_same<Archive, hpx::util::portable_binary_oarchive>::value),
      void>::type
    serialize(Archive & ar, std::tuple<As...>& t, unsigned int version)
    {
      rpc::serialize_tuple_impl<sizeof...(As)>::serialize(ar, t, version);
    }
    
    template<typename... As>
    typename std::enable_if<!std::is_empty<std::tuple<As...> >::value,
                            void>::type
    serialize(hpx::util::portable_binary_iarchive & ar,
              std::tuple<As...>& t, unsigned int version)
    {
      rpc::serialize_tuple_impl<sizeof...(As)>::serialize(ar, t, version);
    }
    
    template<typename... As>
    typename std::enable_if<!std::is_empty<std::tuple<As...> >::value,
                            void>::type
    serialize(hpx::util::portable_binary_oarchive & ar,
              std::tuple<As...>& t, unsigned int version)
    {
      rpc::serialize_tuple_impl<sizeof...(As)>::serialize(ar, t, version);
    }
    
#endif
    
  }
}
// #endif

#define RPC_TUPLE_HH_DONE
#else
#  ifndef RPC_TUPLE_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_TUPLE_HH
