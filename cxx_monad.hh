#ifndef CXX_MONAD_HH
#define CXX_MONAD_HH

#include "cxx_monad_set.hh"
#include "cxx_monad_shared_ptr.hh"
#include "cxx_monad_vector.hh"

namespace cxx {

// unit (and make): m a
// bind:            m a -> (a -> m b) -> m b
// fmap:            (a -> b) -> m a -> m b
// join:            m (m a) -> m a
// zero:            m a
// some:            [a] -> m a
// plus:            [m a] -> m a

namespace monad {

#if 0

// Monadic overloads

template <template <typename> class M, typename T> M<T> unit(T &&x);

template <template <typename> class M, typename T, typename... As>
M<T> make(As &&... as);

template <template <typename> class M, typename R, typename T, typename F>
M<R> bind(const M<T> &m, const F &f);

template <template <typename> class M, typename R, typename T, typename F>
M<R> fmap(const F &f, const M<T> &... m);

template <template <typename> class M, typename T> M<T> join(const M<M<T> > &x);

template <template <typename> class M, typename T> M<T> zero();

template <template <typename> class M, typename T> M<T> some(T &&... xs);

template <template <typename> class M, typename T>
M<T> plus(const M<T> &... ms);

#endif

#if 0

// Sample implementations

template <template <typename> class M, typename R, typename T, typename F>
M<R> bind(const M<T> &x, const F &f) {
  return join<M>(fmap<M>(f, x));
}

template <template <typename> class M, typename R, typename T, typename F>
M<R> fmap(const F &f, const M<T> &x) {
  return bind(x, [f](const T &y) { return unit<M>(std::invoke(f, y)); });
}

template <template <typename> class M, typename T>
M<T> join(const M<M<T> > &x) {
  return bind(x, [](const T &x) { return x; });
}

#endif
}
}

#endif // #ifndef CXX_MONAD_HH
