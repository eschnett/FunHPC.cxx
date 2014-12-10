#ifndef CXX_MAYBE_HH
#define CXX_MAYBE_HH

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_shape_fwd.hh"

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
    return *this;
  }
  maybe &operator=(maybe &&maybe_) {
    assert(this != &maybe_);
    this->~maybe();
    new (this) maybe(std::move(maybe_));
    return *this;
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
  template <typename F, typename... As,
            typename R = typename cxx::invoke_of<F, T, As &&...>::type>
  maybe<R> fmap(const F &f, As &&... as) const {
    return is_nothing()
               ? maybe<R>()
               : maybe<R>(cxx::invoke(f, just(), std::forward<As>(as)...));
  }
  template <typename F, typename R, typename... As>
  typename std::enable_if<
      std::is_same<typename cxx::invoke_of<F, T, As &&...>::type, R>::value,
      R>::type
  fold(const F &f, const R &z, As &&... as) const {
    return is_nothing() ? z : cxx::invoke(f, just(), std::forward<As>(as)...);
  }
};
template <typename T> void swap(maybe<T> &a, maybe<T> &b) { a.swap(b); }

////////////////////////////////////////////////////////////////////////////////

// kinds
template <typename T> struct kinds<maybe<T> > {
  typedef T value_type;
  template <typename U> using constructor = maybe<U>;
};
template <typename T> struct is_maybe : std::false_type {};
template <typename T> struct is_maybe<cxx::maybe<T> > : std::true_type {};

// foldable
template <typename Op, typename R, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<Op, R, R, As...>::type, R>::value,
    R>::type
fold(const Op &op, const R &z, const maybe<R> &xs, const As &... as) {
  bool s = xs.is_just();
  if (s == false)
    return z;
  return xs.just();
}

template <typename T> const T &head(const maybe<T> &xs) {
  assert(xs.is_just());
  return xs.just();
}
template <typename T> const T &last(const maybe<T> &xs) {
  assert(xs.is_just());
  return xs.just();
}

template <typename F, typename Op, typename R, typename T, typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap(const F &f, const Op &op, const R &z, const maybe<T> &xs,
        const As &... as) {
  bool s = xs.is_just();
  if (s == false)
    return z;
  return cxx::invoke(f, xs.just(), as...);
}

template <typename F, typename Op, typename R, typename T, typename T2,
          typename... As>
typename std::enable_if<
    (std::is_same<typename cxx::invoke_of<F, T, T2, As...>::type, R>::value &&
     std::is_same<typename cxx::invoke_of<Op, R, R>::type, R>::value),
    R>::type
foldMap2(const F &f, const R &z, const maybe<T> &xs, const maybe<T2> &ys,
         const As &... as) {
  bool s = xs.is_just();
  assert(ys.is_just() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, xs.just(), ys.just(), as...);
}

#if 0
template <typename F, typename R, typename T, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, As...>::type, R>::value,
    R>::type
foldl(const F &f, const R &z, const maybe<T> &xs, const As &... as) {
  bool s = xs.is_just();
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.just(), as...);
}

template <typename F, typename R, typename T, typename T2, typename... As>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T, T2, As...>::type, R>::value,
    R>::type
foldl2(const F &f, const R &z, const maybe<T> &xs, const maybe<T2> &ys,
       const As &... as) {
  bool s = xs.is_just();
  assert(ys.is_just() == s);
  if (s == false)
    return z;
  return cxx::invoke(f, z, xs.just(), ys.just(), as...);
}
#endif

// functor
template <typename F, typename T, typename... As, typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, As...>::type>
C<R> fmap(const F &f, const cxx::maybe<T> &xs, const As &... as) {
  bool s = xs.is_just();
  if (s == false)
    return C<R>();
  return C<R>(cxx::invoke(f, xs.just(), as...));
}

template <typename F, typename T, typename T2, typename... As,
          typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<F, T, T2, As...>::type>
C<R> fmap2(const F &f, const cxx::maybe<T> &xs, const cxx::maybe<T2> &ys,
           const As &... as) {
  bool s = xs.is_just();
  assert(ys.is_just() == s);
  if (s == false)
    return C<R>();
  return C<R>(cxx::invoke(f, xs.just(), ys.just(), as...));
}

template <
    typename F, typename T, typename B, std::ptrdiff_t D, typename... As,
    typename CT = cxx::maybe<T>,
    template <typename> class C = cxx::kinds<CT>::template constructor,
    typename R = typename cxx::invoke_of<F, T, boundaries<B, D>, As...>::type>
C<R> fmap_boundaries(const F &f, const cxx::maybe<T> &xs,
                     const boundaries<cxx::maybe<B>, D> &bss,
                     const As &... as) {
  bool s = xs.is_just();
  if (s == false)
    return C<R>();
  assert(foldMap([](const cxx::maybe<B> &bs) { return bs.is_just(); },
                 std::logical_and<bool>(), true, bss));
  return C<R>(cxx::invoke(
      f, xs.just(),
      fmap([](const cxx::maybe<B> &bs) { return bs.just(); }, bss), as...));
}

// template <typename F, typename T, typename... As>
// auto boundary(const F &f, const maybe<T> &xs, std::ptrdiff_t dir, bool face,
//               const As &... as) {
//   assert(xs.is_just());
//   return fmap(f, xs, dir, face, as...);
// }

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return C<T>(std::forward<T1>(x));
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  return C<T>(T(std::forward<As>(as)...));
}

template <typename T, typename F, typename... As, typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const cxx::maybe<T> &xs, const F &f, As &&... as) {
  if (xs.is_nothing())
    return C<R>();
  return cxx::invoke(f, xs.just(), std::forward<As>(as)...);
}

template <typename T, typename CCT = cxx::maybe<cxx::maybe<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const cxx::maybe<cxx::maybe<T> > &xss) {
  if (xss.is_nothing())
    return C<T>();
  return xss.just();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = cxx::maybe<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
mplus(const cxx::maybe<T> &xs, const As &... as) {
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    if (xss[i]->is_just())
      return *xss[i];
  return mzero<C, T>();
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_maybe<C<T> >::value, C<T> >::type msome(T &&x) {
  return munit<T>(std::forward<T>(x));
}

// iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...>,
          std::enable_if_t<cxx::is_maybe<C<T> >::value> * = nullptr>
auto iota(const F &f, const iota_range_t &range, const As &... as) {
  std::ptrdiff_t s = range.local.size();
  assert(s == 1);
  return munit<C>(cxx::invoke(f, range.local.imin, as...));
}

template <template <typename> class C, typename F, std::ptrdiff_t D,
          typename... As,
          typename T = cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
          std::enable_if_t<cxx::is_maybe<C<T> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  std::ptrdiff_t s = range.size();
  assert(s == 1);
  return munit<C>(cxx::invoke(f, global_range, range.imin(), as...));
}
}

#define CXX_MAYBE_HH_DONE
#else
#ifndef CXX_MAYBE_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_MAYBE_HH
