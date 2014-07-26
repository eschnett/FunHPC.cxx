#ifndef CXX_MONAD_HH
#define CXX_MONAD_HH

#include "cxx_monad_function.hh"
#include "cxx_monad_list.hh"
#include "cxx_monad_set.hh"
#include "cxx_monad_shared_ptr.hh"
#include "cxx_monad_vector.hh"

#include "cxx_functor.hh"

namespace cxx {

// unit: m a
// make: m a
// bind: m a -> (a -> m b) -> m b
// fmap: (a -> b) -> m a -> m b
// join: m (m a) -> m a
// zero: m a
// plus: [m a] -> m a
// some: [a] -> m a

// TODO: append one entry (push_back)?
// TODO: introduce iota function

// TODO: introduce cxx_applicative?
//       pure (unit)
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

    template <typename T1, typename T = typename std::decay<T1>::type> C<T> unit(T1 &&x);

template <template <typename> class C, typename T, typename... As>
C<T> make(As &&... as);

template <template <typename> class C, typename R, typename T, typename F>
C<R> bind(const C<T> &xs, const F &f);

template <template <typename> class C, typename R, typename T, typename F>
C<R> fmap(const F &f, const C<T> &... xs);

template <template <typename> class C, typename T>
C<T> join(const C<C<T> > &xss);

template <template <typename> class C, typename T> C<T> zero();

template <template <typename> class C, typename T>
C<T> plus(const C<T> &xs, const As &... as);

template <template <typename> class C, typename T, typename... As>
C<T> some(const C<T> &xs, const As &... as);
  };

#endif

#if 0

// Sample implementations

// bind :: C a -> (a -> C b) -> C b
// bind xs f = join (fmap f xs)
template <template <typename> class C, typename R, typename T, typename F>
C<R> bind(const C<T> &xs, const F &f) {
  return join(fmap(f, xs));
}

// fmap :: (a -> b) -> C a -> C b
// fmap f xs = bind xs (unit . f)
// fmap f xs = bind xs (\x -> unit (f x))
template <template <typename> class C, typename R, typename T, typename F>
C<R> fmap(const F &f, const C<T> &xs) {
  return bind(xs, [f](const T &y) { return unit<C>(std::invoke(f, y)); });
}

// join :: C (C a) -> C a
// join xss = bind xss id
// join xss = bind xss (\x -> x)
template <template <typename> class C, typename T>
C<T> join(const C<C<T> > &xss) {
  return bind(xss, [](const T &xs) { return xs; });
}

template <template <typename> class C, typename T, typename... As>
C<T> some(T &&x,  As &&... as) {
  return plus(unit<C>(std::forward<T>(x)), unit<C>(std::forward<As>(as))...);
}

#endif
}

#endif // #ifndef CXX_MONAD_HH
