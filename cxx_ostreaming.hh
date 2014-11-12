#ifndef CXX_OSTREAMING_HH
#define CXX_OSTREAMING_HH

#include "cxx_foldable.hh"
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

// TODO: make "ostreamer" a template parameter as well?
// TODO: rename to "writer"? or "writing"?

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

// TODO: Rename this to "output"?
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

// monad: munit
// Turn a regular function in a function that is ostreaming (outputting nothing)
template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_ostreaming<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return { ostreamer(), std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_ostreaming<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  return { ostreamer(), T(std::forward<As>(as)...) };
}

// monad: mbind
// Bind a function to an ostreaming
// TODO: decay invoke_of?
template <typename T, typename F, typename... As,
          typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_ostreaming<CR>::value, C<R> >::type
mbind(const cxx::ostreaming<T> &xs, const F &f, const As &... as) {
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
// C<R> mbind(cxx::ostreaming<T> &&xs, const F &f) {
//   T &value = xs.value;
//   ostreaming<R> result = cxx::invoke(f, std::move(value));
//   ostreamer &left = xs.ostr;
//   ostreamer &right = result.ostr;
//   ostreamer combined = std::move(left) << std::move(right);
//   return { std::move(combined), std::move(result.value) };
// }
template <typename T, typename F, typename... As,
          typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_ostreaming<CR>::value, C<R> >::type
operator>>=(const cxx::ostreaming<T> &xs, const F &f) {
  return cxx::mbind(xs, f);
}

template <typename T, typename R, typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<R> mbind0(const cxx::ostreaming<T> &xs, const cxx::ostreaming<R> &rs) {
  return cxx::mbind(xs, [rs](const T &) { return rs; });
}
template <typename T, typename R, typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<R> operator>>(const cxx::ostreaming<T> &xs, const cxx::ostreaming<R> &rs) {
  return cxx::mbind0(xs, rs);
}

// monad: mjoin
// Combine two ostreamings
template <typename T, typename CCT = ostreaming<ostreaming<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const ostreaming<ostreaming<T> > &xss) {
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
// C<T> mjoin(ostreaming<ostreaming<T> > &&xss) {
//   ostreamer &outer = xss.ostr;
//   ostreamer &inner = xss.value.ostr;
//   ostreamer combined = std::move(outer) << std::move(inner);
//   T &value = xss.value.value;
//   return { std::move(combined), std::move(value) };
// }

// mapM :: Monad m => (a -> m b) -> [a] -> m [b]
// mapM_ :: Monad m => (a -> m b) -> [a] -> m ()
template <typename F, typename IT, typename... As,
          typename T = typename IT::value_type,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_ostreaming<CR>::value, C<std::tuple<> > >::type
mapM_(const F &f, const IT &xs, const As &... as) {
  std::cout << "ostreaming::mapM_.0 F=" << typeid(F).name()
            << " IT=" << typeid(IT).name() << " sz=" << xs.size() << "\n";
  assert(xs.size() <= 1000);
  ostreamer ostr;
#warning "TODO: Use foldable instead of iterators"
  size_t i = 0;
  for (const T &x : xs) {
    std::cout << "ostreaming::mapM_.1 i=" << i++ << "\n";
    C<R> ys = cxx::invoke(f, x, as...);
    std::cout << "ostreaming::mapM_.2\n";
    // ostr = std::move(ostr) << std::move(ys.ostr);
    ostr = ostr << ys.ostr;
  }
  std::cout << "ostreaming::mapM_.9\n";
  return C<std::tuple<> >(std::move(ostr), std::tuple<>());
}

// // sequence :: Monad m => [m a] -> m [a]
// // sequence_ :: Monad m => [m a] -> m ()
// template <typename ICT,
//           template <typename> class I = cxx::kinds<ICT>::template
//           constructor,
//           typename CT = typename cxx::kinds<ICT>::value_type,
//           template <typename> class C = cxx::kinds<CT>::template constructor,
//           typename T = typename cxx::kinds<CT>::value_type>
// typename std::enable_if<cxx::is_ostreaming<CT>::value, C<std::tuple<> >
// >::type
// sequence_(const ICT &xss) {
//   C<std::tuple<> > rs;
//   for (const C<T> &xs : xss)
//     for (const T &x : xs)
//       if (!rs)
//         rs = munit<C>(std::tuple<>());
//   return std::move(rs);
// }

// mvoid :: Functor f => f a -> f ()
template <typename T, typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<std::tuple<> > mvoid(const cxx::ostreaming<T> &xs) {
  return C<std::tuple<> >(xs.ostr, std::tuple<>());
}
template <typename T, typename CT = cxx::ostreaming<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<std::tuple<> > mvoid(cxx::ostreaming<T> &&xs) {
  return C<std::tuple<> >(std::move(xs.ostr), std::tuple<>());
}

// // foldM :: Monad m => (a -> b -> m a) -> a -> [b] -> m a
// template <typename F, typename R, typename IT, typename... As,
//           typename T = typename cxx::kinds<IT>::value_type,
//           typename CR = typename cxx::invoke_of<F, R, T, As...>::type,
//           template <typename> class C = cxx::kinds<CR>::template constructor,
//           typename R1 = typename cxx::kinds<CR>::value_type,
//           template <typename> class I = cxx::kinds<IT>::template constructor>
// typename std::enable_if<
//     cxx::is_ostreaming<CR>::value && std::is_same<R1, R>::value, C<R> >::type
// foldM(const F &f, const R &z, const IT &xs, const As &... as) {
//   C<R> rs;
//   for (const T &x : xs) {
//     C<R> ys = cxx::invoke(f, z, x, as...);
//     std::move(ys.begin(), ys.end(), std::inserter(rs, rs.end()));
//   }
//   return std::move(rs);
// }

// // foldM_ :: Monad m => (a -> b -> m a) -> a -> [b] -> m ()
// template <typename F, typename R, typename IT, typename... As,
//           typename T = typename cxx::kinds<IT>::value_type,
//           typename CR = typename cxx::invoke_of<F, R, T, As...>::type,
//           template <typename> class C = cxx::kinds<CR>::template constructor,
//           typename R1 = typename cxx::kinds<CR>::value_type,
//           template <typename> class I = cxx::kinds<IT>::template constructor>
// typename std::enable_if<
//     cxx::is_ostreaming<CR>::value && std::is_same<R1, R>::value, C<R> >::type
// foldM_(const F &f, const R &z, const IT &xs, const As &... as) {
//   C<std::tuple<> > rs;
//   for (const T &x : xs) {
//     C<R> ys = cxx::invoke(f, z, x, as...);
//     if (!ys.empty())
//       if (!rs)
//         rs = munit<C>(std::tuple<>());
//   }
//   return std::move(rs);
// }

// liftM :: Monad m => (a1 -> r) -> m a1 -> m r

// liftM2 :: Monad m => (a1 -> a2 -> r) -> m a1 -> m a2 -> m r

// ap :: Monad m => m (a -> b) -> m a -> m b
}

#endif // #ifndef CXX_OSTREAMING_HH
