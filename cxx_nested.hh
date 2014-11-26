#ifndef CXX_NESTED_HH
#define CXX_NESTED_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

#include <tuple>

namespace cxx {

// This assumes that the inner monad is "simple", e.g. a pointer
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct nested {
  typedef T value_type;
  Outer<Inner<T> > values;

  nested() : values(mzero<Outer, Inner<T> >()) {}
  nested(const T &x) : nested(munit<Inner>(x)) {}
  nested(T &&x) : nested(munit<Inner>(std::move(x))) {}
  nested(const Inner<T> &x) : values(munit<Outer>(x)) {}
  nested(Inner<T> &&x) : values(munit<Outer>(std::move(x))) {}
  nested(const Outer<Inner<T> > &xs) : values(xs) {}
  nested(Outer<Inner<T> > &&xs) : values(std::move(xs)) {}
  void swap(Outer<Inner<T> > &xs) { swap(*this, xs); }

  struct mmake : std::tuple<> {};
  template <typename... As>
  nested(mmake, As &&... as)
      : values(munit<Outer>(cxx::mmake<Inner, T>(std::forward<As>(as)...))) {}
};

template <typename T, template <typename> class Outer,
          template <typename> class Inner>
void swap(nested<T, Outer, Inner> &x, nested<T, Outer, Inner> &y) {
  x.swap(y);
}

// kinds

template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct kinds<nested<T, Outer, Inner> > {
  typedef T value_type;
  template <typename U> using constructor = nested<U, Outer, Inner>;
};
template <typename T> struct is_nested : std::false_type {};
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct is_nested<nested<T, Outer, Inner> > : std::true_type {};

// foldable

template <typename F, typename Op, typename R, typename T,
          template <typename> class Outer, template <typename> class Inner,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z,
             const nested<T, Outer, Inner> &xs, const As &... as) {
  return foldMap([&f, &op, &z](const Inner<T> &xs, const As &... as) {
                   return foldMap(f, op, z, xs, as...);
                 },
                 op, z, xs.values, as...);
}

template <typename F, typename Op, typename R, typename T,
          template <typename> class Outer, template <typename> class Inner,
          typename T2, typename... As>
auto foldMap2(const F &f, const Op &op, const R &z,
              const nested<T, Outer, Inner> &xs,
              const nested<T2, Outer, Inner> &ys, const As &... as) {
  return foldMap2(
      [&f, &op, &z](const Inner<T> &xs, const Inner<T2> &ys, const As &... as) {
        return foldMap2(f, op, z, xs, ys, as...);
      },
      op, z, xs.values, ys.values, as...);
}

template <typename Op, typename R, typename T, template <typename> class Outer,
          template <typename> class Inner, typename... As>
auto fold(const Op &op, const R &z, const nested<T, Outer, Inner> &xs,
          const As &... as) {
  return foldMap([](const T &x) { return x; }, op, z, xs, as...);
}

// functor

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename... As>
auto fmap(const F &f, const nested<T, Outer, Inner> &xs, const As &... as) {
  typedef typename cxx::invoke_of<F, T, As...>::type R;
  return nested<R, Outer, Inner>(fmap(
      [&f](const Inner<T> &xs, const As &... as) { return fmap(f, xs, as...); },
      xs.values));
}

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename T2, typename... As>
auto fmap2(const F &f, const nested<T, Outer, Inner> &xs,
           const nested<T2, Outer, Inner> &ys, const As &... as) {
  typedef typename cxx::invoke_of<F, T, T2, As...>::type R;
  return nested<R, Outer, Inner>(
      fmap2([&f](const Inner<T> &xs, const Inner<T2> &ys,
                 const As &... as) { return fmap2(f, xs, ys, as...); },
            xs.values, ys.values, as...));
}

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename T2, typename T3,
          typename... As>
auto fmap3(const F &f, const nested<T, Outer, Inner> &xs,
           const nested<T2, Outer, Inner> &ys,
           const nested<T3, Outer, Inner> &zs, const As &... as) {
  typedef typename cxx::invoke_of<F, T, T2, T3, As...>::type R;
  return nested<R, Outer, Inner>(
      fmap3([&f](const Inner<T> &xs, const Inner<T2> &ys, const Inner<T3> &zs,
                 const As &... as) { return fmap3(f, xs, ys, zs, as...); },
            xs.values, ys.values, zs.values, as...));
}

template <typename F, typename G, typename T, template <typename> class Outer,
          template <typename> class Inner, typename B, typename... As>
auto stencil_fmap(const F &f, const G &g, const nested<T, Outer, Inner> &xs,
                  const B &bm, const B &bp, const As &... as) {
  typedef typename cxx::invoke_of<F, T, B, B, As...>::type R;
  static_assert(
      std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value, "");
  return nested<R, Outer, Inner>(stencil_fmap(
      [&f](const Inner<T> &px, const Inner<B> &bm, const Inner<B> &bp,
           const As &... as) { return fmap3(f, px, bm, bp, as...); },
      [&g](const Inner<T> &px, bool face) { return fmap(g, px, face); },
      xs.values, munit<Inner>(bm), munit<Inner>(bp), as...));
}

// monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type>
typename std::enable_if<cxx::is_nested<C<T> >::value, C<T> >::type
munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As>
typename std::enable_if<cxx::is_nested<C<T> >::value, C<T> >::type
mmake(As &&... as) {
  return C<T>(typename C<T>::mmake(), std::forward<As>(as)...);
}

template <typename T, template <typename> class Outer,
          template <typename> class Inner, typename F, typename... As>
auto mbind(const nested<T, Outer, Inner> &xs, const F &f, const As &... as) {
  typedef typename cxx::invoke_of<F, T, As...>::type CR;
  static_assert(is_nested<CR>::value, "");
  typedef typename CR::value_type R;
  return CR(mbind(xs.values,
                  [&f](const Inner<T> &ys, const As &... as) {
                    return foldMap([&f](const T &x, const As &... as) {
                                     return cxx::invoke(f, x, as...).values;
                                   },
                                   [](const auto &xs,
                                      const auto &ys) { return mplus(xs, ys); },
                                   mzero<Outer, Inner<R> >(), ys, as...);
                  },
                  as...));
}

template <typename T, template <typename> class Outer,
          template <typename> class Inner,
          typename CT = nested<T, Outer, Inner>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
auto mjoin(const nested<nested<T, Outer, Inner>, Outer, Inner> &xss) {
  // join xss = bind xss (\x->x)
  return CT(mbind(xss.values, [](const Inner<CT> &xs) {
    return foldMap([](const CT &x) { return x.values; },
                   [](const auto &xs, const auto &ys) { return mplus(xs, ys); },
                   mzero<Outer, Inner<T> >(), xs);
  }));
}

template <typename T, template <typename> class Outer,
          template <typename> class Inner, typename... Ts,
          typename CT = nested<T, Outer, Inner>,
          template <typename> class C = cxx::kinds<CT>::template constructor>
auto mplus(const nested<T, Outer, Inner> &xs,
           const nested<Ts, Outer, Inner> &... yss) {
  static_assert(cxx::all<std::is_same<T, Ts>::value...>::value, "");
  return C<T>(mplus(xs.values, yss.values...));
}

template <template <typename> class C, typename T>
typename std::enable_if<cxx::is_nested<C<T> >::value, C<T> >::type mzero() {
  return C<T>();
}
}

#endif // #ifndef CXX_NESTED_HH
