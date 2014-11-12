#ifndef CXX_MONAD_VECTOR_HH
#define CXX_MONAD_VECTOR_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"

#include "cxx_invoke.hh"
#include "cxx_kinds.hh"

#include <array>
#include <iterator>
#include <vector>
#include <type_traits>

namespace cxx {

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  C<T> rs;
  rs.emplace_back(std::forward<As>(as)...);
  return rs;
}

template <typename T, typename F, typename... As, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_vector<CR>::value, C<R> >::type
mbind(const std::vector<T> &xs, const F &f, const As &... as) {
  C<R> rs;
  for (const auto &x : xs) {
    C<R> y = cxx::invoke(f, x, as...);
    std::move(y.begin(), y.end(), std::inserter(rs, rs.end()));
  }
  return rs;
}
template <typename T, typename F, typename... As, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_vector<CR>::value, C<R> >::type
operator>>=(const std::vector<T> &xs, const F &f) {
  return cxx::mbind(xs, f);
}

template <typename T, typename R, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<R> mbind0(const std::vector<T> &, const std::vector<R> &rs) {
  return rs;
}
template <typename T, typename R, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<R> operator>>(const std::vector<T> &xs, const std::vector<R> &rs) {
  return cxx::mbind0(xs, rs);
}

template <typename T, typename CCT = std::vector<std::vector<T> >,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::value_type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor>
C<T> mjoin(const std::vector<std::vector<T> > &xss) {
  C<T> rs;
  for (const auto &xs : xss) {
    rs.insert(rs.end(), xs.begin(), xs.end());
  }
  return rs;
}

// mapM :: Monad m => (a -> m b) -> [a] -> m [b]
// mapM_ :: Monad m => (a -> m b) -> [a] -> m ()
template <typename F, typename IT, typename... As,
          typename T = typename IT::value_type,
          typename CR = typename cxx::invoke_of<F, T, As...>::type,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R = typename cxx::kinds<CR>::value_type>
typename std::enable_if<cxx::is_vector<CR>::value, C<std::tuple<> > >::type
mapM_(const F &f, const IT &xs, const As &... as) {
  C<std::tuple<> > rs;
  for (const T &x : xs) {
    C<R> ys = cxx::invoke(f, x, as...);
    for (const R &y : ys)
      rs.push_back(std::tuple<>());
  }
  return std::move(rs);
}

// sequence :: Monad m => [m a] -> m [a]
// sequence_ :: Monad m => [m a] -> m ()
template <typename ICT,
          template <typename> class I = cxx::kinds<ICT>::template constructor,
          typename CT = typename cxx::kinds<ICT>::value_type,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::value_type>
typename std::enable_if<cxx::is_vector<CT>::value, C<std::tuple<> > >::type
sequence_(const ICT &xss) {
  C<std::tuple<> > rs;
  for (const C<T> &xs : xss)
    for (const T &x : xs)
      rs.push_back(std::tuple<>());
  return std::move(rs);
}

// mvoid :: Functor f => f a -> f ()
template <typename T, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
C<std::tuple<> > mvoid(const std::vector<T> &xs) {
  std::size_t s = xs.size();
  C<std::tuple<> > rs(s);
  for (size_t i = 0; i < xs.size(); ++i)
    rs[i] = std::tuple<>();
  return rs;
}

// foldM :: Monad m => (a -> b -> m a) -> a -> [b] -> m a
template <typename F, typename R, typename IT, typename... As,
          typename T = typename cxx::kinds<IT>::value_type,
          typename CR = typename cxx::invoke_of<F, R, T, As...>::type,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R1 = typename cxx::kinds<CR>::value_type,
          template <typename> class I = cxx::kinds<IT>::template constructor>
typename std::enable_if<cxx::is_vector<CR>::value && std::is_same<R1, R>::value,
                        C<R> >::type
foldM(const F &f, const R &z, const IT &xs, const As &... as) {
  C<R> rs;
  for (const T &x : xs) {
    C<R> ys = cxx::invoke(f, z, x, as...);
    std::move(ys.begin(), ys.end(), std::inserter(rs, rs.end()));
  }
  return std::move(rs);
}

// foldM_ :: Monad m => (a -> b -> m a) -> a -> [b] -> m ()
template <typename F, typename R, typename IT, typename... As,
          typename T = typename cxx::kinds<IT>::value_type,
          typename CR = typename cxx::invoke_of<F, R, T, As...>::type,
          template <typename> class C = cxx::kinds<CR>::template constructor,
          typename R1 = typename cxx::kinds<CR>::value_type,
          template <typename> class I = cxx::kinds<IT>::template constructor>
typename std::enable_if<cxx::is_vector<CR>::value && std::is_same<R1, R>::value,
                        C<R> >::type
foldM_(const F &f, const R &z, const IT &xs, const As &... as) {
  C<std::tuple<> > rs;
  for (const T &x : xs) {
    C<R> ys = cxx::invoke(f, z, x, as...);
    for (const R &y : ys)
      rs.push_back(std::tuple<>());
  }
  return rs;
}

// liftM :: Monad m => (a1 -> r) -> m a1 -> m r

// liftM2 :: Monad m => (a1 -> a2 -> r) -> m a1 -> m a2 -> m r

// ap :: Monad m => m (a -> b) -> m a -> m b

// MonadPlus

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}

template <typename T, typename... As, typename CT = std::vector<T>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
typename std::enable_if<cxx::all<std::is_same<As, C<T> >::value...>::value,
                        C<T> >::type
mplus(const std::vector<T> &xs, const As &... as) {
  C<T> rs(xs);
  std::array<const C<T> *, sizeof...(As)> xss{ { &as... } };
  for (size_t i = 0; i < xss.size(); ++i)
    rs.insert(rs.end(), xss[i]->begin(), xss[i]->end());
  return rs;
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_vector<C<T> >::value &&
                            cxx::all<std::is_same<As, T>::value...>::value,
                        C<T> >::type
msome(const T &x, const As &... as) {
  C<T> rs;
  rs.push_back(x);
  std::array<const T *, sizeof...(As)> xs{ { &as... } };
  for (size_t i = 0; i < xs.size(); ++i)
    rs.push_back(*xs[i]);
  return rs;
}

template <typename FCT,
          template <typename> class F = cxx::kinds<FCT>::template constructor,
          typename CT = typename cxx::kinds<FCT>::value_type,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::value_type>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
msum(const FCT &xss) {
  // msum = foldr mmplus mmzero
  return cxx::fold(mplus<T>, mzero<C, T>);
}

template <template <typename> class C, typename IT,
          template <typename> class I = cxx::kinds<IT>::template constructor,
          typename T = typename cxx::kinds<IT>::value_type>
typename std::enable_if<cxx::is_vector<C<T> >::value, C<T> >::type
mfold(const IT &xs) {
  // mfold = mfromList . Foldable.toList
  C<T> rs;
  for (const T &x : xs)
    rs.push_back(x);
  return rs;
}
}

#endif // #ifndef CXX_MONAD_VECTOR_HH
