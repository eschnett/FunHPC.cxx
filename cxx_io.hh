#ifndef CXX_IO
#define CXX_IO

#include "cxx_invoke.hh"

#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cxx {

// A function that writes something to a stream
class writer {
  std::function<void(std::ostream &)> fun;

  template <typename T> static auto to_string(const T &x) {
    std::ostringstream os;
    os << x;
    return os.str();
  }

public:
  writer() : fun([](std::ostream &os) {}) {}
  writer(writer &&w1, writer &&w2)
      : fun([ f1 = std::move(w1.fun), f2 = std::move(w2.fun) ](
            std::ostream & os) { f1(os), f2(os); }) {}
  writer(std::string &&s)
      : fun([s = std::move(s)](std::ostream & os) { os << s; }) {}
  template <typename T> writer(const T &x) : writer(to_string(x)) {}
  writer(writer &&w) : fun(std::move(w.fun)) {}
  writer &operator=(writer &&w) { return fun = std::move(w.fun), *this; }
  void operator()(std::ostream &os) const { fun(os); }
};

// A monad that contains a writer
template <typename T> class IO {
public:
  typedef T value_type;
  T elt;
  writer w;

  IO(const T &elt) : elt(elt) {}
  IO(T &&elt) : elt(std::move(elt)) {}
  IO(const T &elt, writer &&w) : elt(elt), w(std::move(w)) {}
  IO(T &&elt, writer &&w) : elt(std::move(elt)), w(std::move(w)) {}

  T get(std::ostream &os) { return w(os), std::move(elt); }
};

template <typename T> auto make_IO(T &&x) { return IO<T>(x); }
auto make_IO() { return IO<std::tuple<> >(std::tuple<>()); }

template <typename T, typename F, typename IOU = cxx::invoke_of_t<F, T> >
auto bind_IO(IO<T> &&x, const F &f) {
  auto y = cxx::invoke(f, std::move(x.elt));
  return IOU(std::move(y.elt), writer(std::move(x.w), std::move(y.w)));
}
template <typename F, typename T, typename IOU = cxx::invoke_of_t<F> >
auto bind0_IO(IO<T> &&x, const F &f) {
  auto y = cxx::invoke(f);
  return IOU(std::move(y.elt), writer(std::move(x.w), std::move(y.w)));
}
template <typename T, typename U> auto bind1_IO(IO<T> &&x, IO<U> &&y) {
  return IO<U>(std::move(y.elt), writer(std::move(x.w), std::move(y.w)));
}

template <typename T, typename F, typename IOU = cxx::invoke_of_t<F, T> >
auto operator>>=(IO<T> &&x, const F &f) {
  return bind_IO(std::move(x), f);
}
template <typename F, typename T, typename IOU = cxx::invoke_of_t<F> >
auto operator>>(IO<T> &&x, const F &f) {
  return bind0_IO(std::move(x), f);
}

template <typename F, typename T> auto lift_IO(const F &f, IO<T> &&x) {
  typedef cxx::invoke_of_t<F, T> R;
  return IO<R>(cxx::invoke(f, std::move(x.elt)), std::move(x.w));
}
template <typename F, typename T1, typename T2>
auto lift_IO(const F &f, IO<T1> &&x1, IO<T2> &&x2) {
  typedef cxx::invoke_of_t<F, T1, T2> R;
  return IO<R>(cxx::invoke(f, std::move(x1.elt), std::move(x2.elt)),
               writer(std::move(x1.w), std::move(x2.w)));
}

template <typename U> IO<std::tuple<> > output(const U &x) {
  return IO<std::tuple<> >(std::tuple<>(), writer(x));
}
}

#define CXX_IO_HH_DONE
#else
#ifndef CXX_IO_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_IO_HH
