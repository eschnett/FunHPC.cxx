#ifndef CXX_MONAD_HH
#define CXX_MONAD_HH

#include "cxx_monad_function.hh"
#include "cxx_monad_list.hh"
#include "cxx_monad_set.hh"
#include "cxx_monad_shared_ptr.hh"
#include "cxx_monad_vector.hh"

#include "cxx_functor.hh"

namespace cxx {

// munit: m a
// mmake: m a
// mbind: m a -> (a -> m b) -> m b
// fmap: (a -> b) -> m a -> m b
// mjoin: m (m a) -> m a
// mzero: m a
// mplus: [m a] -> m a
// msome: [a] -> m a

// TODO: append one entry (push_back)?
// TODO: introduce iota function

// TODO: introduce cxx_applicative?
//       pure (munit)
//       <*>
//       also: sequenceA

// TODO: introduce functions that take a reference to the output as
// first argument, so that the return value does not need to be
// copied? is this necessary? test this with some compilers first.

// TODO: introduce rpc::action<> equivalent to std::function<>, and
// make this a monad as well

// Monad functions

#if 0
  template <template <typename> class C> struct monad {

    template <typename T1, typename T = typename std::decay<T1>::type> C<T> munit(T1 &&x);

template <template <typename> class C, typename T, typename... As>
C<T> mmake(As &&... as);

template <template <typename> class C, typename R, typename T, typename... As,
          typename F>
C<R> mbind(const C<T> &xs, const F &f, const As &... as);

    template <template <typename> class C, typename R, typename T, typename... As, typename F>
C<R> fmap(const F &f, const C<T> &xs, const As&... as);

template <template <typename> class C, typename T>
C<T> mjoin(const C<C<T> > &xss);

template <template <typename> class C, typename T> C<T> mzero();

    template <template <typename> class C, typename T, typename...As>
C<T> mplus(const C<T> &xs, const As &... as);

template <template <typename> class C, typename T, typename... As>
C<T> msome(const C<T> &xs, const As &... as);
  };

#endif

#if 0

// Sample implementations

// mbind :: C a -> (a -> C b) -> C b
// mbind xs f = mjoin (fmap f xs)
  template <template <typename> class C, typename R, typename T, typename...As,typename F>
  C<R> mbind(const C<T> &xs, const F &f, As&&...as) {
    return mjoin(fmap(f, xs, std::forward<As>(as)...));
}

// fmap :: (a -> b) -> C a -> C b
// fmap f xs = mbind xs (munit . f)
// fmap f xs = mbind xs (\x -> munit (f x))
  template <template <typename> class C, typename R, typename T, typename...As,typename F>
C<R> fmap(const F &f, const C<T> &xs,  As&&...as) {
    return mbind(xs, [f](const T &y) { return munit<C>(std::invoke(f, y)); },std::forward<As>(as)...);
}

// mjoin :: C (C a) -> C a
// mjoin xss = mbind xss id
// mjoin xss = mbind xss (\x -> x)
template <template <typename> class C, typename T>
C<T> mjoin(const C<C<T> > &xss) {
  return mbind(xss, [](const T &xs) { return xs; });
}

template <template <typename> class C, typename T, typename... As>
C<T> msome(T &&x,  As &&... as) {
  return mplus(munit<C>(std::forward<T>(x)), munit<C>(std::forward<As>(as))...);
}

#endif
}

#endif // #ifndef CXX_MONAD_HH
