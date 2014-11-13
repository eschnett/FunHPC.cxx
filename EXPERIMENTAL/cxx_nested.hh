#ifndef CXX_NESTED_HH
#define CXX_NESTED_HH

#error                                                                         \
    "We need monad transformers to do this -- look at the mtl library, and at monadLib"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace cxx {

// Combine two monads

// Definition
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct nested {
  typedef T value_type;
  template <typename U> using outer_constructor = Outer<T>;
  template <typename U> using inner_constructor = Inner<T>;
  template <typename U> static Outer<U> outer_munit(U &&x) {
    return munit<Outer>(std::forward<U>(x));
  }
  template <typename U> static Inner<U> inner_munit(U &&x) {
    return munit<Inner>(std::forward<U>(x));
  }
  // template <typename U, typename... As>
  // static Outer<U> outer_mmake(As &&... as) {
  //   return mmake<Outer>(std::forward<As>(as)...);
  // }
  template <typename U, typename... As>
  static Inner<U> inner_mmake(As &&... as) {
    return mmake<Inner, U>(std::forward<As>(as)...);
  }
  Outer<Inner<T> > values;
};

// kinds
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct kinds<cxx::nested<T, Outer, Inner> > {
  typedef T value_type;
  template <typename U> using constructor = cxx::nested<U, Outer, Inner>;
};
template <typename T> struct is_nested : std::false_type {};
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct is_nested<nested<T, Outer, Inner> > : std::true_type {};

// foldable
template <typename R, typename T, template <typename> class Outer,
          template <typename> class Inner, typename F,
          typename CT = cxx::nested<T, Outer, Inner>,
          template <typename> class C = kinds<CT>::template constructor>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<F, R, T>::type, R>::value, R>::type
foldl(const F &f, const R &z, const cxx::nested<T, Outer, Inner> &xs) {
  return foldl([f](const R &z, const Inner<T> &ys) { return foldl(f, z, ys); },
               z, xs.values);
}

// functor
// fmap :: (a -> b) -> C a -> C b
// fmap :: (a -> b) -> O (I a) -> O (I b)
// fmap f xs = fmapO (fmapI f) xs
namespace detail {
template <typename T> struct unwrap_nested {
  typedef T type;
  typedef T inner_type;
  const T &operator()(const T &x) const { return x; }
};
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct unwrap_nested<cxx::nested<T, Outer, Inner> > {
  typedef T type;
  typedef Inner<T> inner_type;
  const Outer<Inner<T> > &
  operator()(const cxx::nested<T, Outer, Inner> &x) const {
    return x.values;
  }
};
}
template <typename T, template <typename> class Outer,
          template <typename> class Inner, typename... As, typename F,
          typename CT = cxx::nested<T, Outer, Inner>,
          template <typename> class C = kinds<CT>::template constructor,
          typename R = typename cxx::invoke_of<
              F, T, typename detail::unwrap_nested<As>::type...>::type>
C<R> fmap(const F &f, const cxx::nested<T, Outer, Inner> &xs,
          const As &... as) {
  return C<R>{ fmap(
      [f](const Inner<T> &ys,
          const typename detail::unwrap_nested<As>::inner_type &... as) {
        return fmap(f, ys, as...);
      },
      xs.values, detail::unwrap_nested<As>()(as)...) };
}

// monad

// munit :: a -> C a
// munit :: a -> O (I a)
// munit a = munitO (munitI a)
template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_nested<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return C<T>{ C<T>::outer_munit /*munit<C<T>::outer_constructor>*/ (
      C<T>::inner_munit /*munit<C<T>::inner_constructor>*/ (
          std::forward<T1>(x))) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_nested<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  return C<T>{ C<T>::outer_munit(
      C<T>::template inner_mmake<T>(std::forward<As>(as)...)) };
}

// munit :: a -> O (I a)
// munitI :: a -> I a
// munitO :: a -> O a

// fmap :: (a -> b) -> O (I a) -> O (I b)
// fmapI :: (a -> b) -> I a -> I b
// fmapO :: (a -> b) -> O a -> O b

// mbindI :: I a -> (a -> I b) -> I b
// mbindO :: O a -> (a -> O b) -> O b

// mjoinI :: I (I a) -> I a
// mjoinO :: O (O a) -> O b

// mbind :: O (I a) -> (a -> O (I b)) -> O (I b)
// mbind xs f =
//      xs :: O (I a)
//      f  :: a -> O (I b)
//      munitI . f :: a -> I (O (I b))
//      munitO . f :: a -> O (O (I b))
//      fmapI f :: I a -> I (O (I b))
//      fmapO f :: O a -> O (O (I b))
//      munitO . fmapI f :: I a -> O (I (O (I b)))
//      fmapO (munitI . f) :: O a -> O (I (O (I b)))
//      mbindO :: O a     -> (a   -> O b            ) -> O b
//      mbindO :: O (I a) -> (I a -> O (I (O (I b)))) -> O (I (O (I b)))

// [previous] mbind xs f = mbind (mbind xs f) (\ys -> fmap f ys)

// [generic] mbind xs f = mjoin (fmap f xs)

template <typename T, template <typename> class Outer,
          template <typename> class Inner, typename F,
          typename CT = cxx::nested<T, Outer, Inner>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T>::type,
          typename R = typename cxx::kinds<CR>::value_type>
C<R> mbind(const cxx::nested<T, Outer, Inner> &xs, const F &f) {
  return C<R>{ cxx::mbind(cxx::mbind(xs.values, f), [f](const Inner<T> &ys) {
    return cxx::fmap(f, ys);
  }) };
}

#if 0

// mjoin :: C (C a) -> C a
// mjoin :: O (I (O (I a))) -> O (I a)
// mjoin xss = mbind xss id

// mjoin xss =
//      xss :: O (I (O (I a)))
template <template <typename> class M1, template <typename> class M2,
          typename T>
typename std::enable_if<((detail::is_cxx_monad<M1, M2<T> >::value) &&
                         (detail::is_cxx_monad<M2, T>::value)),
                        M1<M2<T> > >::type
mjoin(const M1<M2<M1<M2<T> > > > &x) {
  return mjoin<M2>(x);
}

#endif
}

#endif // #ifndef CXX_NESTED_HH
