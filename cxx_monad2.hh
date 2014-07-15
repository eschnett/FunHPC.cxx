#ifndef CXX_MONAD2_HH
#define CXX_MONAD2_HH

namespace cxx {

// combining two monads
namespace monad {

template <template <typename> class M1, template <typename> class M2,
          typename T>
struct monad2 {
  M1<M2<T> > values;
};

namespace detail {
template <typename T> struct is_cxx_monad2 : std::false_type {};
template <template <typename> class M1, template <typename> class M2,
          typename T>
struct is_cxx_monad2<monad2<M1, M2, T> > : std::true_type {};
}

// unit :: a -> M a
// unit :: a -> M1 (M2 a)
// unit a = unit2 (unit1 a)
template <template <typename> class M1, template <typename> class M2,
          typename T>
typename std::enable_if<detail::is_cxx_monad2<M1, M2, T>::value,
                        monad2<M1, M2, typename std::decay<T>::type> >::type
unit(T &&x) {
  return { unit<M1, M2<typename std::decay<T>::type> >(
      unit<M2, typename std::decay<T>::type>(std::forward<T>(x))) };
}

template <template <typename> class M1, template <typename> class M2,
          typename T, typename... As>
typename std::enable_if<detail::is_cxx_monad2<M1, M2, T>::value,
                        monad2<M1, M2, typename std::decay<T>::type> >::type
make(As &&... as) {
  return { make<M1, M2<T> >(make<M2, T>(std::forward<As>(as)...)) };
}

// bind :: M a -> (a -> M b) -> M b
// bind :: M1 (M2 a) -> (a -> M1 (M2 b)) -> M1 (M2 b)
// bind m f =
template <template <typename> class M1, template <typename> class M2,
          typename R, typename T, typename F>
typename std::enable_if<
    ((detail::is_cxx_monad2<M1, M2, T>::value) &&
     (std::is_same<typename cxx::invoke_of<F, T>::type, R>::value)),
    monad2<M1, M2, R> >::type
bind(const monad2<M1, M2, T> &x, const F &f) {
  return bind<M1>(bind<M2>(x, f), fmap(f));
}

// fmap :: (a -> b) -> M a -> M b
// fmap :: (a -> b) -> M1 (M2 a) -> M1 (M2 b)
// fmap f m = fmap1 (\m' -> fmap2 f m') m
namespace detail {
template <typename T> struct unwrap_cxx_monad2 {
  typedef T type;
};
template <template <typename> class M, typename T>
struct unwrap_cxx_monad2<M<T> > {
  typedef T type;
};
}
template <template <typename> class M1, template <typename> class M2,
          typename R, typename... As, typename F>
typename std::enable_if<
    ((detail::is_cxx_monad2<M1, M2, T>::value) &&
     (std::is_same<typename invoke_of<F, As...>::type, R>::value)),
    monad2<M1, M2, R> >::type
fmap(const F &f, const As &... as) {
  return fmap<M1>([f](const typename detail::unwrap_cxx_monad2<
                      M1, As>::type &... as) { return fmap<M2>(f, as...); },
                  as...);
}

// join :: M (M a) -> M a
// join :: M1 (M2 (M1 (M2 a))) -> M1 (M2 a)
// join m = bind m (\x -> x)
template <template <typename> class M1, template <typename> class M2,
          typename T>
typename std::enable_if<((detail::is_cxx_monad<M1, M2<T> >::value) &&
                         (detail::is_cxx_monad<M2, T>::value)),
                        M1<M2<T> > >::type
join(const M1<M2<M1<M2<T> > > > &x) {
  return join<M2>(x);
}
}
}

#endif // #ifndef CXX_MONAD2_HH
