#ifndef CXX_EITHER_HH
#define CXX_EITHER_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <cereal/access.hpp>

#include <array>
#include <cassert>
#include <type_traits>
#include <utility>

namespace cxx {

// An either-or class which can hold either of two different types

// TODO: add constructors that choose explicitly between left and
// right, so that left and right can hold the same type

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

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(is_left_);
    if (is_left_)
      ar(left_);
    else
      ar(right_);
  }

public:
  either() = delete;
  explicit either(const L &left_) : is_left_(true), left_(left_) {}
  explicit either(L &&left_) : is_left_(true), left_(std::move(left_)) {}
  explicit either(const R &right_) : is_left_(false), right_(right_) {}
  explicit either(R &&right_) : is_left_(false), right_(std::move(right_)) {}
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
    return *this;
  }
  either &operator=(either &&either_) {
    assert(this != &either_);
    this->~either();
    new (this) either(std::move(either_));
    return *this;
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
  either<typename cxx::invoke_of<F, L, As&&...>::type,
         typename cxx::invoke_of<G, R, As&&...>::type>
  gmap(F f, G g, As &&... as) const {
    typedef typename cxx::invoke_of<F, L, As...>::type FL;
    typedef typename cxx::invoke_of<G, R, As...>::type GR;
    return is_left() ? either<FL, GR>(
                           cxx::invoke(f, left(), std::forward<As>(as)...))
                     : either<FL, GR>(
                           cxx::invoke(g, right(), std::forward<As>(as)...));
  }
#endif
private:
  template <typename T> struct unwrap_either {
    typedef T left_type;
    typedef T right_type;
    const T &left(const T &x) const { return x; }
    const T &right(const T &x) const { return x; }
  };
  template <typename L1, typename R1> struct unwrap_either<either<L1, R1> > {
    typedef L1 left_type;
    typedef R1 right_type;
    const L1 &left(const either<L1, R1> &x) const { return x.left(); }
    const R1 &right(const either<L1, R1> &x) const { return x.right(); }
  };

public:
  struct gmap : std::tuple<> {};
  template <typename F, typename G, typename L1, typename R1, typename... As>
  either(
      typename std::enable_if<
          ((std::is_same<typename cxx::invoke_of<
                             F, L1, typename unwrap_either<typename std::decay<
                                        As>::type>::left_type...>::type,
                         L>::value) &&
           (std::is_same<typename cxx::invoke_of<
                             G, R1, typename unwrap_either<typename std::decay<
                                        As>::type>::right_type...>::type,
                         R>::value)),
          gmap>::type,
      const F &f, const G &g, const either<L1, R1> &x, As &&... as) {
    if (x.is_left()) {
      is_left_ = true;
      new (&left_) L(cxx::invoke(
          f, x.left(), unwrap_either<typename std::decay<As>::type>().left(
                           std::forward<As>(as))...));
    } else {
      is_left_ = false;
      new (&right_) R(cxx::invoke(
          g, x.right(), unwrap_either<typename std::decay<As>::type>().right(
                            std::forward<As>(as))...));
    }
  }
  // TODO: make gfoldl more uniform to foldl, with expecting a zero element
  template <typename F, typename G, typename... As>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, L, As &&...>::type,
                   typename cxx::invoke_of<G, R, As &&...>::type>::value,
      typename cxx::invoke_of<G, R, As &&...>::type>::type
  gfoldl(F f, G g, As &&... as) const {
    return is_left() ? cxx::invoke(f, left(), std::forward<As>(as)...)
                     : cxx::invoke(g, right(), std::forward<As>(as)...);
  }
};
template <typename L, typename R> void swap(either<L, R> &a, either<L, R> &b) {
  a.swap(b);
}

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename L, typename R> struct kinds<either<L, R> > {
  typedef R element_type;
  template <typename U> using constructor = either<L, U>;
};
template <typename T> struct is_either : std::false_type {};
template <typename L, typename R>
struct is_either<either<L, R> > : std::true_type {};

// foldable
template <typename R, typename T, typename L, typename F>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const either<L, T> &xs) {
  return xs.is_left() ? z : cxx::invoke(f, z, xs.right());
}

// functor
namespace detail {
template <typename T> struct unwrap_either {
  typedef T type;
  const T &operator()(const T &x) const { return x; }
};
template <typename T, typename L> struct unwrap_either<either<L, T> > {
  typedef T type;
  const T &operator()(const either<L, T> &x) const { return x.right(); }
};
}
template <typename T, typename L, typename... As, typename F,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_either<As>::type...>::type>
C<R> fmap(const F &f, const cxx::either<L, T> xs, const As &... as) {
  if (!xs.is_right())
    return C<R>(typename C<R>::left_type());
  return C<R>(cxx::invoke(f, xs.right(), detail::unwrap_either<As>()(as)...));
}

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_either<C<T> >::value, C<T> >::type
unit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_either<C<T> >::value, C<T> >::type
make(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, typename L, typename F, typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::element_type>
C<R> bind(const cxx::either<L, T> &xs, const F &f) {
  if (xs.is_left())
    return C<R>(xs.left());
  return cxx::invoke(f, xs.right());
}

template <typename T, typename L,
          typename CCT = cxx::either<L, cxx::either<L, T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::element_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> join(const cxx::either<L, cxx::either<L, T> > &xss) {
  if (xss.is_left())
    return C<T>(xss.left());
  return xss.right();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_either<C<T> >::value, C<T> >::type zero() {
  return C<T>(typename C<T>::left_type());
}

template <typename T, typename L, typename... As,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
plus(const cxx::either<L, T> &xs, const As &... as) {
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    if (xss[i]->is_right())
      return *xss[i];
  return zero<C, T>();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_either<C<T> >::value, C<T> >::type some(T &&x) {
  return unit<C>(std::forward<T>(x));
}
}

#endif // #ifndef CXX_EITHER_HH
