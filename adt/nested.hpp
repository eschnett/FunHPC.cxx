#ifndef ADT_NESTED_HPP
#define ADT_NESTED_HPP

#include <type_traits>
#include <utility>

namespace adt {

// nested<P,A,T> = P<A<T>>
template <template <typename> class P, template <typename> class A, typename T>
struct nested {
  template <typename U> using pointer_constructor = P<U>;
  template <typename U> using array_constructor = A<U>;
  typedef T value_type;
  P<A<T>> data;
  bool invariant() const {
    if (fun::mempty(data))
      return true;
    if (!fun::mempty(fun::mextract(data)))
      return false;
    return true;
  }
};
}

namespace fun {

// is_nested

namespace detail {
template <typename> struct is_nested : std::false_type {};
template <template <typename> class P, template <typename> class A, typename T>
struct is_nested<adt::nested<P, A, T>> : std::true_type {};
}

// iota

template <template <typename> class C, typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::ptrdiff_t,
                                        std::decay_t<Args>...>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> iota(F &&f, std::ptrdiff_t s, Args &&... args) {
  return {iota<P>([s](std::ptrdiff_t, auto &&f,
                      auto &&... args) { return iota<A>(f, s, args...); },
                  std::ptrdiff_t(1), std::forward<F>(f),
                  std::forward<Args>(args)...)};
}

// fmap

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
adt::nested<P, A, R> fmap(F &&f, const adt::nested<P, A, T> &xss,
                          Args &&... args) {
  return {fmap([](const A<T> &xs, auto &&f,
                  auto &&... args) { return fmap(f, xs, args...); },
               xss.data, std::forward<F>(f), std::forward<Args>(args)...)};
}

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename T2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
adt::nested<P, A, R> fmap2(F &&f, const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, T2> &yss, Args &&... args) {
  return {fmap2([](const A<T> &xs, const A<T2> &ys, auto &&f,
                   auto &&... args) { return fmap2(f, xs, ys, args...); },
                xss.data, yss.data, std::forward<F>(f),
                std::forward<Args>(args)...)};
}

// foldMap

template <typename F, typename Op, typename Z, template <typename> class P,
          template <typename> class A, typename T, typename... Args,
          typename R = cxx::invoke_of_t<F, T, Args...>>
R foldMap(F &&f, Op &&op, const Z &z, const adt::nested<P, A, T> &xss,
          Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return foldMap([](const A<T> &xs, auto &&f, auto &&op, const Z &z,
                    auto &&... args) { return foldMap(f, op, z, xs, args...); },
                 op, z, xss.data, std::forward<F>(f), op, z,
                 std::forward<Args>(args)...);
}

// munit

template <template <typename> class C, typename T, typename R = std::decay_t<T>,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor,
          std::enable_if_t<detail::is_nested<C<R>>::value> * = nullptr>
C<R> munit(T &&x) {
  return {munit<P>(munit<A>(std::forward<T>(x)))};
}

// mjoin

template <template <typename> class P, template <typename> class A, typename T>
adt::nested<P, A, T>
mjoin(const adt::nested<P, A, adt::nested<P, A, T>> &xssss) {
  return {fmap([](const A<adt::nested<P, A, T>> &xsss) {
                 return mjoin(fmap([](const adt::nested<P, A, T> &xss) {
                                     return mextract(xss.data);
                                   },
                                   xsss));
               },
               xssss.data)};
}

// mbind

template <typename F, template <typename> class P, template <typename> class A,
          typename T, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>,
          typename R = typename CR::value_type>
adt::nested<P, A, R> mbind(F &&f, const adt::nested<P, A, T> &xss,
                           Args &&... args) {
  return mjoin(fmap(std::forward<F>(f), xss, std::forward<Args>(args)...));
}

// mextract

template <template <typename> class P, template <typename> class A, typename T>
decltype(auto) mextract(const adt::nested<P, A, T> &xss) {
  return mextract(mextract(xss.data));
}

// mzero

template <template <typename> class C, typename R,
          template <typename> class P = C<R>::template pointer_constructor,
          template <typename> class A = C<R>::template array_constructor>
C<R> mzero() {
  return {mzero<P, A<R>>()};
}

// mplus

template <template <typename> class P, template <typename> class A, typename T,
          typename... Ts>
adt::nested<P, A, T> mplus(const adt::nested<P, A, T> &xss,
                           const adt::nested<P, A, Ts> &... yss) {
  return {munit<P, A<T>>(
      mplus(mempty(xss.data) ? mzero<A, T>() : mextract(xss.data),
            mempty(yss.data) ? mzero<A, T>() : mextract(yss.data)...))};
}

// mempty

template <template <typename> class P, template <typename> class A, typename T>
bool mempty(const adt::nested<P, A, T> &xss) {
  return mempty(xss.data);
}
}

#define ADT_NESTED_HPP_DONE
#endif // #ifdef ADT_NESTED_HPP
#ifndef ADT_NESTED_HPP_DONE
#error "Cyclic include dependency"
#endif
