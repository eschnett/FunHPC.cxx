#ifndef CXX_OSTREAMING_HH
#define CXX_OSTREAMING_HH

#include "cxx_functor.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

#include <cereal/access.hpp>
#include <cereal/types/string.hpp>

#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace cxx {

// An ostreamer, a function that outputs something
class ostreamer {
  typedef std::function<std::ostream &(std::ostream &)> fun_t;
  std::shared_ptr<fun_t> fun;

  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const {
    std::ostringstream os;
    (*fun)(os);
    ar(os.str());
  }
  template <typename Archive> void load(Archive &ar) {
    std::string s;
    ar(s);
    *this =
        ostreamer([s](std::ostream & os) -> std::ostream & { return os << s; });
  }

public:
  ostreamer(const fun_t &fun) : fun(std::make_shared<fun_t>(fun)) {}
  ostreamer(fun_t &&fun) : fun(std::make_shared<fun_t>(std::move(fun))) {}
  ostreamer()
      : ostreamer([](std::ostream & os) -> std::ostream & { return os; }) {}
  std::ostream &operator()(std::ostream &os) const { return (*fun)(os); }
};

// Functions for ostreamers

// Create an ostreamer from something that can be output
template <typename Elem> ostreamer make_ostreamer(const Elem &e) {
  return ostreamer([e](std::ostream & os) -> std::ostream &
                   { return os << e; });
}
template <typename Elem> ostreamer make_ostreamer_str(const Elem &e) {
  std::ostringstream oss;
  oss << e;
  std::string s = oss.str();
  return ostreamer([s](std::ostream & os) -> std::ostream &
                   { return os << s; });
}

// Combine two ostreamers
// (This is just function composition)
ostreamer operator<<(const ostreamer &left, const ostreamer &right) {
  return ostreamer([ left, right ](std::ostream & os) -> std::ostream &
                   { return right(left(os)); });
}

// Convenience wrapper
template <typename Elem>
ostreamer operator<<(const ostreamer &os, const Elem &e) {
  return os << make_ostreamer(e);
}

// Abstractions for ostreamers

// A container that holds an element of type Elem, while remembering
// what should have been output while calculating it
template <typename T> class ostreaming {
public: // TODO
  ostreamer ostr;
  T value;

  ostreaming(const ostreamer &ostr, const T &value)
      : ostr(ostr), value(value) {}
  ostreaming(const ostreamer &ostr, T &&value)
      : ostr(ostr), value(std::move(value)) {}

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(ostr, value); }

public:
  ostreaming() {} // only for serialize
  ostreaming(const T &value) : ostr(), value(value) {}
  ostreaming(T &&value) : ostr(), value(std::move(value)) {}
  const T &get(std::ostream &os) const {
    ostr(os);
    return value;
  }
  T &&get(std::ostream &os) {
    ostr(os);
    return std::move(value);
  }
};

cxx::ostreaming<std::tuple<> > put(const cxx::ostreamer &os) {
  return { os, std::tuple<>() };
}
cxx::ostreaming<std::tuple<> > put(cxx::ostreamer &&os) {
  return { std::move(os), std::tuple<>() };
}
template <typename Elem> cxx::ostreaming<std::tuple<> > put(const Elem &elem) {
  return { make_ostreamer(elem), std::tuple<>() };
}

// kinds

template <typename T> struct kinds<cxx::ostreaming<T> > {
  typedef T value_type;
  template <typename U> using constructor = cxx::ostreaming<U>;
};
template <typename T> struct is_ostreaming : std::false_type {};
template <typename T>
struct is_ostreaming<cxx::ostreaming<T> > : std::true_type {};

// functor: fmap
// Turn a regular function into a function that acts on an ostreaming
template <typename F, typename T, typename... As,
          typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, const cxx::ostreaming<T> &ostr, As &&... as) {
  auto result = cxx::invoke(f, ostr.value, std::forward<As>(as)...);
  return { ostr.ostr, std::move(result) };
}
template <typename F, typename T, typename... As,
          typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T>::type>
C<R> fmap(const F &f, cxx::ostreaming<T> &&ostr, As &&... as) {
  auto result = cxx::invoke(f, std::move(ostr.value), std::forward<As>(as)...);
  return { std::move(ostr.ostr), std::move(result) };
}

// monad: unit
// Turn a regular function in a function that is ostreaming (outputting nothing)
template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_ostreaming<C<T> >::value, C<T> >::type
unit(T1 &&x) {
  return { ostreamer(), std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_ostreaming<C<T> >::value, C<T> >::type
make(As &&... as) {
  return { ostreamer(), T(std::forward<As>(as)...) };
}

// monad: bind
// Bind a function to an ostreaming
template <typename T, typename F, typename... As,
          typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = /*TODO: why? */ typename std::decay<
              typename cxx::invoke_of<F, T, As...>::type>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> bind(const cxx::ostreaming<T> &xs, const F &f, const As &... as) {
  const T &value = xs.value;
  ostreaming<R> result = cxx::invoke(f, value, as...);
  const ostreamer &left = xs.ostr;
  ostreamer &right = result.ostr;
  ostreamer combined = left << std::move(right);
  return { std::move(combined), std::move(result.value) };
}
// template <typename T, typename F, typename CT = cxx::ostreaming<T>,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename CR = typename cxx::invoke_of<F, T &&>::type,
//           typename R = typename cxx::kinds<CR>::value_type>
// C<R> bind(cxx::ostreaming<T> &&xs, const F &f) {
//   T &value = xs.value;
//   ostreaming<R> result = cxx::invoke(f, std::move(value));
//   ostreamer &left = xs.ostr;
//   ostreamer &right = result.ostr;
//   ostreamer combined = std::move(left) << std::move(right);
//   return { std::move(combined), std::move(result.value) };
// }

// monad: join
// Combine two ostreamings
template <typename T, typename CCT = ostreaming<ostreaming<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const ostreaming<ostreaming<T> > &xss) {
  const ostreamer &outer = xss.ostr;
  const ostreamer &inner = xss.value.ostr;
  ostreamer combined = outer << inner;
  const T &value = xss.value.value;
  return { std::move(combined), value };
}
// template <typename T, typename CCT = ostreaming<ostreaming<T> >,
//           template <typename> class C = cxx::kinds<CCT>::template
// constructor,
//           typename CT = typename cxx::kinds<CCT>::value_type,
//           template <typename> class C2 = cxx::kinds<CT>::template
// constructor>
// C<T> join(ostreaming<ostreaming<T> > &&xss) {
//   ostreamer &outer = xss.ostr;
//   ostreamer &inner = xss.value.ostr;
//   ostreamer combined = std::move(outer) << std::move(inner);
//   T &value = xss.value.value;
//   return { std::move(combined), std::move(value) };
// }
}

#endif // #ifndef CXX_OSTREAMING_HH
