#ifndef CXX_EITHER_HH
#define CXX_EITHER_HH

#include "cxx_invoke.hh"

#include <algorithm>
#include <array>
#include <cassert>
#include <type_traits>
#include <utility>

namespace cxx {

// An either-or class which can hold either of two different types

template <typename L, typename R> class either {
public:
  typedef L left_type;
  typedef R right_type;

private:
  bool is_left_;
  union {
    L left_;
    R right_;
  };

public:
  either() = delete;
  either(const L &left_) : is_left_(true), left_(left_) {}
  either(L &&left_) : is_left_(true), left_(std::move(left_)) {}
  either(const R &right_) : is_left_(false), right_(right_) {}
  either(R &&right_) : is_left_(false), right_(std::move(right_)) {}
  either(const either &either_) : is_left_(either_.is_left_) {
    if (is_left_)
      new (&left_) L(either_.left_);
    else
      new (&right_) R(either_.right_);
  }
  either(either &&either_) : is_left_(either_.is_left_) {
    if (is_left_)
      new (&left_) L(std::move(either_.left_));
    else
      new (&right_) R(std::move(either_.right_));
  }
  ~either() {
    if (is_left_)
      left_.~L();
    else
      right_.~R();
  }
  either &operator=(const either &either_) {
    if (this == &either_)
      return *this;
    this->~either();
    new (this) either(either_);
  }
  either &operator=(either &&either_) {
    assert(this != either_);
    this->~either();
    new (this) either(std::move(either_));
  }
  void swap(either &either_) {
    either tmp(std::move(either_));
    either_ = std::move(*this);
    *this = std::move(tmp);
  }
  bool is_left() const { return is_left_; }
  bool is_right() const { return !is_left(); }
  const L &left() const {
    assert(is_left());
    return left_;
  }
  L &left() {
    assert(is_left());
    return left_;
  }
  const R &right() const {
    assert(is_right());
    return right_;
  }
  R &right() {
    assert(is_right());
    return right_;
  }
// operator bool() const { return is_right(); }
// const T& operator*() const { return right(); }
// T& operator*() { return right(); }
#if 0
  template <typename F, typename G, typename... As>
  either<typename ::cxx::invoke_of<F, L, As&&...>::type,
         typename ::cxx::invoke_of<G, R, As&&...>::type>
  gmap(F f, G g, As &&... as) const {
    typedef typename ::cxx::invoke_of<F, L, As...>::type FL;
    typedef typename ::cxx::invoke_of<G, R, As...>::type GR;
    return is_left() ? either<FL, GR>(
                           ::cxx::invoke(f, left(), std::forward<As>(as)...))
                     : either<FL, GR>(
                           ::cxx::invoke(g, right(), std::forward<As>(as)...));
  }
#endif
private:
  template <typename T> struct unwrap_either {
    typedef T left_type;
    typedef T right_type;
    bool is_left(const T &x) const { return true; }
    bool is_right(const T &x) const { return true; }
    const T &left(const T &x) const { return x; }
    const T &right(const T &x) const { return x; }
  };
  template <typename L1, typename R1> struct unwrap_either<either<L1, R1> > {
    typedef L1 left_type;
    typedef R1 right_type;
    bool is_left(const either<L1, R1> &x) const { return x.is_left(); }
    bool is_right(const either<L1, R1> &x) const { return x.is_right(); }
    const L1 &left(const either<L1, R1> &x) const { return x.left(); }
    const R1 &right(const either<L1, R1> &x) const { return x.right(); }
  };

public:
  struct gmap : std::tuple<> {};
  template <typename F, typename G, typename... As>
  either(
      typename std::enable_if<
          ((std::is_same<typename ::cxx::invoke_of<
                             F, typename unwrap_either<As>::left_type...>::type,
                         L>::value) &&
           (std::is_same<
               typename ::cxx::invoke_of<
                   G, typename unwrap_either<As>::right_type...>::type,
               R>::value)),
          gmap>::type,
      F f, G g, const As &... as) {
    std::array<bool, sizeof...(As)> is_lefts{ { unwrap_either<As>().is_left(
        as)... } };
    bool is_left = *std::min_element(is_lefts.begin(), is_lefts.end());
    std::array<bool, sizeof...(As)> is_rights{ { unwrap_either<As>().is_right(
        as)... } };
    bool is_right = *std::min_element(is_rights.begin(), is_rights.end());
    // If there are no eithers, treat it as left
    if (is_left) {
      is_left_ = true;
      new (&left_) L(cxx::invoke(f, unwrap_either<As>().left(as)...));
    } else if (is_right) {
      is_left_ = false;
      new (&right_) R(cxx::invoke(g, unwrap_either<As>().right(as)...));
    } else {
      is_left_ = true;
      new (&left_) L();
    }
  }
  template <typename F, typename G, typename... As>
  typename std::enable_if<
      std::is_same<typename ::cxx::invoke_of<F, L, As &&...>::type,
                   typename ::cxx::invoke_of<G, R, As &&...>::type>::value,
      typename ::cxx::invoke_of<G, R, As &&...>::type>::type
  gfoldl(F f, G g, As &&... as) const {
    return is_left() ? ::cxx::invoke(f, left(), std::forward<As>(as)...)
                     : ::cxx::invoke(g, right(), std::forward<As>(as)...);
  }
};
template <typename L, typename R> void swap(either<L, R> &a, either<L, R> &b) {
  a.swap(b);
}

namespace foldable {

template <typename T, typename L, typename R, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, T, R>::type, T>::value, R>::type
foldl(const F &f, const T &z, const either<L, R> &x) {
  return x.is_left() ? z : cxx::invoke(f, z, x.right());
}
}

namespace monad {

namespace detail {
template <typename T> struct is_cxx_either : std::false_type {};
template <typename L, typename R>
struct is_cxx_either<either<L, R> > : std::true_type {};
}

template <template <typename> class M, typename T>
typename std::enable_if<
    detail::is_cxx_either<M<typename std::decay<T>::type> >::value,
    M<typename std::decay<T>::type> >::type
unit(T &&x) {
  return M<typename std::decay<T>::type>(std::forward<T>(x));
}

template <template <typename> class M, typename T, typename... As>
typename std::enable_if<detail::is_cxx_either<M<T> >::value, M<T> >::type
make(As &&... as) {
  return M<T>(T(std::forward<As>(as)...));
}

template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<(detail::is_cxx_either<M<T> >::value &&std::is_same<
                            typename invoke_of<F, T>::type, M<R> >::value),
                        M<R> >::type
bind(const M<T> &x, const F &f) {
  if (x.is_left())
    return M<R>(x.left());
  return ::cxx::invoke(f, x.right());
}

#if 0
template <template <typename> class M, typename R, typename T, typename F>
typename std::enable_if<(detail::is_cxx_either<M<T> >::value &&std::is_same<
                            typename invoke_of<F, T>::type, R>::value),
                        M<R> >::type
fmap(const F &f, const M<T> &x) {
  if (x.is_left())
    return M<R>(x.left());
  return unit<M>(::cxx::invoke(f, x.right()));
}
#endif

namespace detail {
template <typename T> struct unwrap_cxx_either {
  typedef T type;
  bool is_right(const T &x) const { return true; }
  const T &operator()(const T &x) const { return x; }
};
template <typename L, typename R> struct unwrap_cxx_either<either<L, R> > {
  typedef R type;
  bool is_right(const either<L, R> &x) const { return x.is_right(); }
  const R &operator()(const either<L, R> &x) const { return x.right(); }
};
}

template <template <typename> class M, typename R, typename... As, typename F>
typename std::enable_if<
    (detail::is_cxx_either<M<R> >::value &&std::is_same<
        typename ::cxx::invoke_of<
            F, typename detail::unwrap_cxx_either<As>::type...>::type,
        R>::value),
    M<R> >::type
fmap(const F &f, const As &... as) {
  std::array<bool, sizeof...(As)> is_rights{
    { detail::unwrap_cxx_either<As>().is_right(as)... }
  };
  bool is_right = *std::min_element(is_rights.begin(), is_rights.end());
  if (!is_right)
    return M<R>(typename M<R>::left_type());
  return unit<M>(::cxx::invoke(f, detail::unwrap_cxx_either<As>()(as)...));
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_either<M<T> >::value, M<T> >::type
join(const M<M<T> > &x) {
  if (x.is_left())
    return M<T>(x.left());
  return x.right();
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_either<M<T> >::value, M<T> >::type
zero() {
  return M<T>(typename M<T>::left_type());
}

template <template <typename> class M, typename T>
typename std::enable_if<detail::is_cxx_either<M<T> >::value, M<T> >::type
plus(const M<T> &x, const M<T> &y) {
  return x.is_right() ? x : y;
}
}
}

#endif // #ifndef CXX_EITHER_HH
