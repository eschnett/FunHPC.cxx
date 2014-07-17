#ifndef CXX_MAYBE_HH
#define CXX_MAYBE_HH

#include "cxx_invoke.hh"

#include <cassert>
#include <type_traits>
#include <utility>

namespace cxx {

// A maybe class which maybe holds a type, or not

// template<typename T> using maybe = either<void,T>;
template <typename T> class maybe {
  bool is_nothing_;
  union {
    T just_;
  };

public:
  maybe() : is_nothing_(true) {}
  explicit maybe(const T &just_) : is_nothing_(false), just_(just_) {}
  explicit maybe(T &&just_) : is_nothing_(false), just_(std::move(just_)) {}
  maybe(const maybe &maybe_) : is_nothing_(maybe_.is_nothing_) {
    if (!is_nothing_)
      new (&just_) T(maybe_.just_);
  }
  maybe(maybe &&maybe_) : is_nothing_(maybe_.is_nothing_) {
    if (!is_nothing_)
      new (&just_) T(std::move(maybe_.just_));
  }
  ~maybe() {
    if (!is_nothing_)
      just_.~T();
  }
  maybe &operator=(const maybe &maybe_) {
    if (this == &maybe_)
      return;
    this->~maybe();
    new (this) maybe(maybe_);
  }
  maybe &operator=(maybe &&maybe_) {
    assert(this != &maybe_);
    this->~maybe();
    new (this) maybe(std::move(maybe_));
  }
  void swap(maybe &maybe_) {
    maybe tmp(std::move(maybe_));
    maybe_ = std::move(*this);
    *this = std::move(tmp);
  }
  bool is_nothing() const { return is_nothing_; }
  bool is_just() const { return !is_nothing(); }
  const T &just() const {
    assert(is_just());
    return just_;
  }
  T &just() {
    assert(is_just());
    return just_;
  }
  // operator bool() const { return is_just(); }
  // const T& operator*() const { return just(); }
  // T& operator*() { return just(); }
  template <typename R, typename F, typename... As>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, T, As &&...>::type, R>::value,
      maybe<R> >::type
  fmap(F f, As &&... as) const {
    return is_nothing()
               ? maybe<R>()
               : maybe<R>(cxx::invoke(f, just(), std::forward<As>(as)...));
  }
  template <typename R, typename F, typename... As>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, T, As &&...>::type, R>::value,
      R>::type
  fold(const R &z, F f, As &&... as) const {
    return is_nothing() ? z : cxx::invoke(f, just(), std::forward<As>(as)...);
  }
};
template <typename T> void swap(maybe<T> &a, maybe<T> &b) { a.swap(b); }

namespace foldable {

template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const maybe<T> &x) {
  return x.is_nothing() ? z : cxx::invoke(f, z, x.just());
}
}

namespace functor {

namespace detail {
template <typename T> struct is_cxx_maybe : std::false_type {};
template <typename T> struct is_cxx_maybe<maybe<T> > : std::true_type {};
}

namespace detail {
template <typename T> struct unwrap_cxx_maybe {
  typedef T type;
  bool is_just(const T &x) const { return true; }
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_cxx_maybe<maybe<T> > {
  typedef T type;
  bool is_just(const maybe<T> &x) const { return x.is_just(); }
  const T &operator()(const maybe<T> &x) const {
    return x.just();
  };
};
}

template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    (detail::is_cxx_maybe<M<R> >::value &&std::is_same<
        typename cxx::invoke_of<
            F, typename detail::unwrap_cxx_maybe<As>::type...>::type,
        R>::value),
    M<R> >::type
fmap(const F &f, const As &... as) {
  std::array<bool, sizeof...(As)> is_justs{
    { detail::unwrap_cxx_maybe<As>().is_just(as)... }
  };
  bool is_just = *std::min_element(is_justs.begin(), is_justs.end());
  if (!is_just)
    return maybe<R>();
  return M<R>(cxx::invoke(f, detail::unwrap_cxx_maybe<As>()(as)...));
}
}

namespace monad {

using cxx::functor::fmap;

namespace detail {
template <typename T> struct is_cxx_maybe : std::false_type {};
template <typename T> struct is_cxx_maybe<maybe<T> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_cxx_maybe<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return M<typename std::decay<T>::type>(std::forward<T>(x));
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_cxx_maybe<M<T> >::value, M<T> >::type
make(As &&... as) {
  return maybe<T>(T(std::forward<As>(as)...));
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<(detail::is_cxx_maybe<M<T> >::value &&std::is_same<
                            typename cxx::invoke_of<F, T>::type, M<R> >::value),
                        M<R> >::type
bind(const M<T> &x, const F &f) {
  if (x.is_nothing())
    return maybe<R>();
  return cxx::invoke(f, x.just());
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_maybe<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  if (x.is_nothing())
    return maybe<T>();
  return x.just();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_maybe<M<T> >::value, M<T> >::type
zero() {
  return maybe<T>();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_maybe<M<T> >::value, M<T> >::type
plus(const M<T> &x, const M<T> &y) {
  return x.is_just() ? x : y;
}
}
}

#endif // #ifndef CXX_MAYBE_HH
