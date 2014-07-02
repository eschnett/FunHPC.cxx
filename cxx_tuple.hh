#ifndef CXX_TUPLE_HH
#define CXX_TUPLE_HH

#include "cxx_invoke.hh"
#include "cxx_utils.hh"

#include <iostream>
#include <tuple>
#include <type_traits>

namespace rpc {

// template<size_t N>
// struct tuple_bind_impl {
//   template<typename... As>
//   bind ...
// };

// TODO: Make the function the first argument of the tuple?

namespace detail {
template <size_t...> struct seq {};

template <size_t N, size_t... S>
struct make_seq : make_seq<N - 1, N - 1, S...> {};

template <size_t... S> struct make_seq<0, S...> {
  typedef seq<S...> type;
};
}

template <typename F, typename... As, size_t... S>
auto tuple_apply_impl(const F &f, const std::tuple<As...> &t, detail::seq<S...>)
    -> typename rpc::invoke_of<F, As...>::type {
  return rpc::invoke(f, std::get<S>(t)...);
}

template <typename F, typename... As>
auto tuple_apply(const F &f, const std::tuple<As...> &t)
    -> typename rpc::invoke_of<F, As...>::type {
  typename detail::make_seq<sizeof...(As)>::type s;
  return tuple_apply_impl(f, t, s);
}

// template<typename F, typename... As, size_t... S>
// auto tuple_apply_unique_impl(F&& f, std::tuple<As...>&& t, detail::seq<S...>)
// ->
//   typename rpc::invoke_of<F, As...>::type
// {
//   return rpc::invoke(std::forward<F>(f), std::move(std::get<S>(t))...);
// }

// template<typename F, typename... As>
// auto tuple_apply_unique(F&& f, std::tuple<As...>&& t) ->
//   typename rpc::invoke_of<F, As...>::type
// {
//   typename detail::make_seq<sizeof...(As)>::type s;
//   return tuple_apply_unique_impl(std::forward<F>(f), std::move(t), s);
// }

// // input: T&&
// // type: T, T&, or T&&
// // output: const T&, T&, or T&&

// // temporaries are passed by value, and should become rvalue
// // references (or remain temporaries?)
// template<typename T>
// struct anti_decay {
//   typedef T&& type;
// };
// // lvalues should remain lvalues
// template<typename T>
// struct anti_decay<T&> {
//   typedef T& type;
// };
// // const lvalues should remain const lvalues
// template<typename T>
// struct anti_decay<const T&> {
//   typedef const T& type;
// };
// // do we need to handle rvalues as well? but then, decaying them
// // should turn them into values, shouldn't it?

// template<typename F, typename... As, size_t... S>
// auto tuple_apply_unique1_impl
// (typename std::decay<F>::type&& f,
//  std::tuple<typename std::decay<As>::type...>&& t,
//  detail::seq<S...>) ->
//   typename rpc::invoke_of<F, As...>::type
// {
//   // return rpc::invoke(std::move(f), std::get<S>(t)...);
//   // return rpc::invoke(f, std::get<S>(t)...);
//   // return std::move(f)(std::get<S>(std::move(t))...);
//   return f(static_cast<typename anti_decay<As>::type>(std::get<S>(t))...);
//   // return f(std::get<S>(t)...);
// }

// template<typename F, typename... As>
// auto tuple_apply_unique1(typename std::decay<F>::type&& f,
//                          std::tuple<typename std::decay<As>::type...>&& t) ->
//   typename rpc::invoke_of<F, As...>::type
// {
//   typename detail::make_seq<sizeof...(As)>::type s;
//   return tuple_apply_unique1_impl<F, As...>(std::move(f), std::move(t), s);
// }

template <template <typename> class F, typename... As, size_t... S>
auto tuple_map_impl(const std::tuple<As...> &t, detail::seq<S...>)
    -> std::tuple<rpc::invoke_of<F<As>(As)>...> {
  return make_tuple(F<As>()(std::get<S>(t))...);
}

template <template <typename> class F, typename... As>
auto tuple_map(const std::tuple<As...> &t)
    -> std::tuple<rpc::invoke_of<F<As>(As)>...> {
  return tuple_map_impl<F>(t, detail::make_seq<sizeof...(As)>::type());
}

template <size_t N> struct input_tuple_impl {
  template <typename... As>
  static void input(std::istream &is, std::tuple<As...> &t) {
    input_tuple_impl<N - 1>::input(is, t);
    is >> std::get<N - 1>(t);
  }
};

template <> struct input_tuple_impl<0> {
  template <typename... As>
  static void input(std::istream &is, std::tuple<As...> &t) {}
};

template <size_t N> struct output_tuple_impl {
  template <typename... As>
  static void output(std::ostream &os, const std::tuple<As...> &t) {
    output_tuple_impl<N - 1>::output(os, t);
    if (N > 1)
      os << ",";
    os << std::get<N - 1>(t);
  }
};

template <> struct output_tuple_impl<0> {
  template <typename... As>
  static void output(std::ostream &os, const std::tuple<As...> &t) {}
};
}

namespace std {

template <typename... As>
std::istream &operator>>(std::istream &is, std::tuple<As...> &t) {
  rpc::input_tuple_impl<sizeof...(As)>::input(is, t);
  return is;
}

template <typename... As>
std::ostream &operator<<(std::ostream &os, const std::tuple<As...> &t) {
  os << "(";
  rpc::output_tuple_impl<sizeof...(As)>::output(os, t);
  os << ")";
  return os;
}
}

#define CXX_TUPLE_HH_DONE
#else
#ifndef CXX_TUPLE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef CXX_TUPLE_HH
