#ifndef CXX_MAYBE_HH
#define CXX_MAYBE_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <cereal/access.hpp>

#include <array>
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

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(is_nothing_);
    if (!is_nothing_)
      ar(just_);
  }

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

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename T> struct kinds<maybe<T> > {
  typedef T element_type;
  template <typename U> using constructor = maybe<U>;
};
template <typename T> struct is_maybe : std::false_type {};
template <typename T> struct is_maybe<cxx::maybe<T> > : std::true_type {};

// foldable
template <typename R, typename T, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const maybe<T> &x) {
  return x.is_nothing() ? z : cxx::invoke(f, z, x.just());
}

// functor
namespace detail {
template <typename T> struct unwrap_maybe {
  typedef T type;
  const T &operator()(const T &x) const { return x; }
};
template <typename T> struct unwrap_maybe<maybe<T> > {
  typedef T type;
  const T &operator()(const maybe<T> &x) const {
    return x.just();
  };
};
}
template <typename T, typename... As, typename F, typename CT = cxx::maybe<T>,
          template <typename> class C = kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_maybe<As>::type...>::type>
C<R> fmap(const F &f, const cxx::maybe<T> xs, const As &... as) {
  if (!xs.is_just())
    return C<R>();
  return C<R>(cxx::invoke(f, xs.just(), detail::unwrap_maybe<As>()(as)...));
}

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type unit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type
make(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, typename F, typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
C<R> bind(const cxx::maybe<T> &xs, const F &f) {
  if (xs.is_nothing())
    return C<R>();
  return cxx::invoke(f, xs.just());
}

template <typename T, typename CCT = cxx::maybe<cxx::maybe<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::element_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const cxx::maybe<cxx::maybe<T> > &xss) {
  if (xss.is_nothing())
    return C<T>();
  return xss.just();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type zero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
plus(const cxx::maybe<T> &xs, const As &... as) {
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    if (xss[i]->is_just())
      return *xss[i];
  return zero<C, T>();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type some(T &&x) {
  return unit<T>(std::forward<T>(x));
}
}

#endif // #ifndef CXX_MAYBE_HH
