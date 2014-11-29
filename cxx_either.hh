#ifndef CXX_EITHER_HH
#define CXX_EITHER_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_utils.hh"

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
  struct gmap : std::tuple<> {};
  template <typename F, typename G, typename L1, typename R1, typename... As>
  either(gmap, const F &f, const G &g, const either<L1, R1> &x, As &&... as) {
    typedef cxx::invoke_of_t<F, L1, As...> TL;
    typedef cxx::invoke_of_t<G, R1, As...> TR;
    static_assert(std::is_same<TL, L>::value && std::is_same<TR, R>::value, "");
    if (x.is_left()) {
      is_left_ = true;
      new (&left_) L(cxx::invoke(f, x.left(), std::forward<As>(as)...));
    } else {
      is_left_ = false;
      new (&right_) R(cxx::invoke(g, x.right(), std::forward<As>(as)...));
    }
  }
  struct gmap2 : std::tuple<> {};
  template <typename F, typename G, typename L1, typename R1, typename L2,
            typename R2, typename... As>
  either(gmap2, const F &f, const G &g, const either<L1, R1> &x,
         const either<L2, R2> &y, As &&... as) {
    typedef cxx::invoke_of_t<F, L1, L2, As...> TL;
    typedef cxx::invoke_of_t<G, R1, R2, As...> TR;
    static_assert(std::is_same<TL, L>::value && std::is_same<TR, R>::value, "");
    assert(y.is_left() == x.is_left());
    if (x.is_left()) {
      is_left_ = true;
      new (&left_)
          L(cxx::invoke(f, x.left(), y.left(), std::forward<As>(as)...));
    } else {
      is_left_ = false;
      new (&right_)
          R(cxx::invoke(g, x.right(), y.right(), std::forward<As>(as)...));
    }
  }
  // TODO: make gfoldl more uniform to foldl, with expecting a zero element
  // TODO: rename gfoldl to (g)foldMap(1)
  template <typename F, typename G, typename... As>
  decltype(auto) gfoldl(const F &f, const G &g, As &&... as) const {
    typedef cxx::invoke_of_t<F, L, As...> RL;
    typedef cxx::invoke_of_t<G, R, As...> RR;
    static_assert(std::is_same<RL, RR>::value, "");
    return is_left() ? cxx::invoke(f, left(), std::forward<As>(as)...)
                     : cxx::invoke(g, right(), std::forward<As>(as)...);
  }
  template <typename F, typename G, typename L1, typename R1, typename... As>
  decltype(auto) gfoldl2(const F &f, const G &g, const either<L1, R1> &ys,
                         As &&... as) const {
    typedef cxx::invoke_of_t<F, L, L1, As...> RL;
    typedef cxx::invoke_of_t<G, R, R1, As...> RR;
    static_assert(std::is_same<RL, RR>::value, "");
    assert(ys.is_left() == is_left());
    return is_left()
               ? cxx::invoke(f, left(), ys.left(), std::forward<As>(as)...)
               : cxx::invoke(g, right(), ys.right(), std::forward<As>(as)...);
  }
};
template <typename L, typename R> void swap(either<L, R> &a, either<L, R> &b) {
  a.swap(b);
}

template <typename F, typename G, typename L1, typename R1, typename... As>
decltype(auto) gfoldl(const F &f, const G &g, const either<L1, R1> &xs,
                      As &&... as) {
  typedef cxx::invoke_of_t<F, L1, As...> RL;
  typedef cxx::invoke_of_t<G, R1, As...> RR;
  static_assert(std::is_same<RL, RR>::value, "");
  return xs.gfoldl(f, g, std::forward<As>(as)...);
}
template <typename F, typename G, typename L1, typename R1, typename L2,
          typename R2, typename... As>
decltype(auto) gfoldl2(const F &f, const G &g, const either<L1, R1> &xs,
                       const either<L2, R2> &ys, As &&... as) {
  typedef cxx::invoke_of_t<F, L1, L2, As...> RL;
  typedef cxx::invoke_of_t<G, R1, R2, As...> RR;
  static_assert(std::is_same<RL, RR>::value, "");
  return xs.gfoldl(f, g, ys, std::forward<As>(as)...);
}

template <typename F, typename G, typename L1, typename R1, typename... As>
auto gmap(const F &f, const G &g, const either<L1, R1> &xs, As &&... as) {
  typedef cxx::invoke_of_t<F, L1, As...> RL;
  typedef cxx::invoke_of_t<G, R1, As...> RR;
  return either<RL, RR>(typename either<RL, RR>::gmap(), f, g, xs,
                        std::forward<As>(as)...);
}
template <typename F, typename G, typename L1, typename R1, typename L2,
          typename R2, typename... As>
auto gmap2(const F &f, const G &g, const either<L1, R1> &xs,
           const either<L2, R2> &ys, As &&... as) {
  typedef cxx::invoke_of_t<F, L1, L2, As...> RL;
  typedef cxx::invoke_of_t<G, R1, R2, As...> RR;
  return either<RL, RR>(typename either<RL, RR>::gmap2(), f, g, xs, ys,
                        std::forward<As>(as)...);
}

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename L, typename R> struct kinds<either<L, R> > {
  typedef R value_type;
  template <typename U> using constructor = either<L, U>;
};
template <typename T> struct is_either : std::false_type {};
template <typename L, typename R>
struct is_either<either<L, R> > : std::true_type {};

// foldable
template <typename Op, typename R, typename L, typename... As>
std::enable_if_t<std::is_same<cxx::invoke_of_t<Op, R, R, As...>, R>::value, R>
fold(const Op &op, const R &z, const either<L, R> &xs, const As &... as) {
  bool s = xs.is_right();
  if (s == false)
    return z;
  return xs.right();
}

template <typename F, typename Op, typename R, typename T, typename L,
          typename... As>
std::enable_if_t<(std::is_same<cxx::invoke_of_t<F, T, As...>, R>::value &&
                  std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value),
                 R>
foldMap(const F &f, const Op &op, const R &z, const either<L, T> &xs,
        const As &... as) {
  bool s = xs.is_right();
  if (s == false)
    return z;
  return cxx::invoke(f, xs.right(), as...);
}

template <typename F, typename Op, typename R, typename T, typename L,
          typename T2, typename L2, typename... As>
std::enable_if_t<(std::is_same<cxx::invoke_of_t<F, T, T2, As...>, R>::value &&
                  std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value),
                 R>
foldMap2(const F &f, const Op &op, const R &z, const either<L, T> &xs,
         const either<L2, T2> &ys, const As &... as) {
  bool s = xs.is_right();
  assert(ys.is_right() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, xs.right(), ys.right(), as...);
}

#if 0
template <typename F, typename R, typename T, typename L, typename... As>
std::enable_if_t<
    std::is_same<cxx::invoke_of_t<F, R, T, As...>, R>::value,
    R>
foldl(const F &f, const R &z, const either<L, T> &xs, const As &... as) {
  bool s = xs.is_right();
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.right(), as...);
}

template <typename F, typename R, typename T, typename L, typename T2,
          typename L2, typename... As>
std::enable_if_t<
    std::is_same<cxx::invoke_of_t<F, R, T, T2, As...>, R>::value,
    R>
foldl2(const F &f, const R &z, const either<L, T> &xs, const either<L2, T2> &ys,
       const As &... as) {
  bool s = xs.is_right();
  assert(ys.is_right() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.right(), ys.right(), as...);
}
#endif

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
template <typename F, typename T, typename L, typename... As,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = cxx::invoke_of_t<F, T, As...> >
C<R> fmap(const F &f, const cxx::either<L, T> &xs, const As &... as) {
  bool s = xs.is_right();
  if (s == false)
    return C<R>(typename C<R>::left_type());
  return C<R>(cxx::invoke(f, xs.right(), as...));
}

template <typename F, typename T, typename L, typename T2, typename... As,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = cxx::invoke_of_t<F, T, T2, As...> >
C<R> fmap2(const F &f, const cxx::either<L, T> &xs,
           const cxx::either<L, T2> &ys, const As &... as) {
  bool s = xs.is_right();
  assert(ys.is_right() == s);
  if (s == false)
    return C<R>(typename C<R>::left_type());
  return C<R>(cxx::invoke(f, xs.right(), ys.right(), as...));
}

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1> >
std::enable_if_t<cxx::is_either<C<T> >::value, C<T> > munit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
std::enable_if_t<cxx::is_either<C<T> >::value, C<T> > mmake(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, typename L, typename F, typename... As,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = cxx::invoke_of_t<F, T, As...>,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const cxx::either<L, T> &xs, const F &f, As &&... as) {
  if (xs.is_left())
    return C<R>(xs.left(), std::forward<As>(as)...);
  return cxx::invoke(f, xs.right(), std::forward<As>(as)...);
}

template <typename T, typename L,
          typename CCT = cxx::either<L, cxx::either<L, T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const cxx::either<L, cxx::either<L, T> > &xss) {
  if (xss.is_left())
    return C<T>(xss.left());
  return xss.right();
}

template <template <typename> class C, typename T>
std::enable_if_t<cxx::is_either<C<T> >::value, C<T> > mzero() {
  return C<T>(typename C<T>::left_type());
}

template <typename T, typename L, typename... As,
          typename CT = cxx::either<L, T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
std::enable_if_t<cxx::all<std::is_same<As, C<T> >::value...>::value, C<T> >
mplus(const cxx::either<L, T> &xs, const As &... as) {
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    if (xss[i]->is_right())
      return *xss[i];
  return mzero<C, T>();
}

template <template <typename> class C, typename T>
std::enable_if_t<cxx::is_either<C<T> >::value, C<T> > msome(T &&x) {
  return munit<C>(std::forward<T>(x));
}

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...> >
std::enable_if_t<cxx::is_either<C<T> >::value, C<T> >
iota(const F &f, ptrdiff_t imin, ptrdiff_t imax, ptrdiff_t istep,
     const As &... as) {
  return munit<C>(cxx::invoke(f, imin, as...));
}
}

#endif // #ifndef CXX_EITHER_HH
