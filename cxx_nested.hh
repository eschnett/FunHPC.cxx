#ifndef CXX_NESTED_HH
#define CXX_NESTED_HH

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_invoke.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

#include <cstddef>
#include <tuple>

namespace cxx {

// This assumes that the inner monad is "simple", e.g. a pointer
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
struct nested {
  typedef T value_type;
  template <typename U> using outer = Outer<U>;
  template <typename U> using inner = Inner<U>;

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

  // We assume that the Inner container is never empty.
  bool empty() const { return values.empty(); }
  // We assume that the Inner container always contains one element
  std::size_t size() const { return values.size(); }
};

template <typename T, template <typename> class Outer,
          template <typename> class Inner>
void swap(nested<T, Outer, Inner> &x, nested<T, Outer, Inner> &y) {
  x.swap(y);
}

// Kinds

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

// Iota

template <template <typename> class C, typename F, typename... As,
          typename T = cxx::invoke_of_t<F, std::ptrdiff_t, As...>,
          std::enable_if_t<cxx::is_nested<C<T> >::value> * = nullptr>
auto iota(const F &f, const iota_range_t &range, const As &... as) {
  return C<T>(iota<C<T>::template outer>(
      [](std::ptrdiff_t i, const F &f, const iota_range_t &range,
         const As &... as) {
        return iota<C<T>::template inner>(
            f, iota_range_t(range.global, i, i + range.local.istep), as...);
      },
      range, f, range, as...));
}

template <template <typename> class C, std::ptrdiff_t D, typename F,
          typename... As,
          typename T = cxx::invoke_of_t<F, grid_region<D>, index<D>, As...>,
          std::enable_if_t<cxx::is_nested<C<T> >::value> * = nullptr>
auto iota(const F &f, const grid_region<D> &global_range,
          const grid_region<D> &range, const As &... as) {
  return C<T>(iota<C<T>::template outer>(
      [](const grid_region<D> &global_range, const index<D> &i, const F &f,
         const As &... as) {
        grid_region<D> range1(i, i + index<D>::set1(1));
        return iota<C<T>::template inner>(f, global_range, range1, as...);
      },
      global_range, range, f, as...));
}

// Functor

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename... As>
auto fmap(const F &f, const nested<T, Outer, Inner> &xs, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> R;
  return nested<R, Outer, Inner>(fmap(
      [&f](const Inner<T> &xs, const As &... as) { return fmap(f, xs, as...); },
      xs.values, as...));
}

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename T2, typename... As>
auto fmap2(const F &f, const nested<T, Outer, Inner> &xs,
           const nested<T2, Outer, Inner> &ys, const As &... as) {
  typedef cxx::invoke_of_t<F, T, T2, As...> R;
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
  typedef cxx::invoke_of_t<F, T, T2, T3, As...> R;
  return nested<R, Outer, Inner>(
      fmap3([&f](const Inner<T> &xs, const Inner<T2> &ys, const Inner<T3> &zs,
                 const As &... as) { return fmap3(f, xs, ys, zs, as...); },
            xs.values, ys.values, zs.values, as...));
}

template <typename F, typename G, typename T, template <typename> class Outer,
          template <typename> class Inner, typename B, typename... As>
auto stencil_fmap(const F &f, const G &g, const nested<T, Outer, Inner> &xs,
                  const B &bm, const B &bp, const As &... as) {
  typedef cxx::invoke_of_t<F, T, B, B, As...> R;
  static_assert(std::is_same<cxx::invoke_of_t<G, T, bool>, B>::value, "");
  return nested<R, Outer, Inner>(stencil_fmap(
      [&f](const Inner<T> &px, const Inner<B> &bm, const Inner<B> &bp,
           const As &... as) { return fmap3(f, px, bm, bp, as...); },
      [&g](const Inner<T> &px, bool face) { return fmap(g, px, face); },
      xs.values, munit<Inner>(bm), munit<Inner>(bp), as...));
}

// TODO: This is Monad.sequence, after introducing
// pair<T,boundaries<T,D>> as Traversable. This is also fmap7 for D=3.
template <typename T, std::ptrdiff_t D>
rpc::client<std::pair<T, boundaries<T, D> > >
sequence(const rpc::client<T> &xs, const boundaries<rpc::client<T>, D> &bs) {}

template <typename F, typename T, template <typename> class Outer,
          template <typename> class Inner, typename... As>
auto boundary(const F &f, const nested<T, Outer, Inner> &xs, std::ptrdiff_t dir,
              bool face, const As &... as) {
  typedef cxx::invoke_of_t<F, T, std::ptrdiff_t, bool, As...> R;
  return nested<R, Outer, Inner>(
      boundary([](const Inner<T> &ys, std::ptrdiff_t dir, bool face, const F &f,
                  const As &... as) { return fmap(f, ys, dir, face, as...); },
               xs.values, dir, face, f, as...));
}

template <typename F, typename G, typename T, template <typename> class Outer,
          template <typename> class Inner, typename B, std::ptrdiff_t D,
          typename... As>
auto stencil_fmap(const F &f, const G &g, const nested<T, Outer, Inner> &xs,
                  const boundaries<nested<B, Outer, Inner>, D> &bs,
                  const As &... as) {
  typedef cxx::invoke_of_t<F, T, boundaries<B, D>, As...> R;
  static_assert(
      std::is_same<cxx::invoke_of_t<G, T, std::ptrdiff_t, bool>, B>::value, "");
  return nested<R, Outer, Inner>(stencil_fmap(
      [&f, &g](const Inner<T> &px, const boundaries<Inner<B>, D> &bs,
               const As &... as) { return fmap_boundaries(f, px, bs, as...); },
      [&g](const Inner<T> &px, std::ptrdiff_t dir,
           bool face) { return fmap(g, px, dir, face); },
      xs.values, fmap([](const auto &b) { return b.values; }, bs), as...));
}

// Foldable

template <typename F, typename Op, typename R, typename T,
          template <typename> class Outer, template <typename> class Inner,
          typename... As>
auto foldMap(const F &f, const Op &op, const R &z,
             const nested<T, Outer, Inner> &xs, const As &... as) {
  return foldMap([&f, &op, &z](const Inner<T> &x, const As &... as) {
                   return foldMap(f, op, z, x, as...);
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

template <typename T, template <typename> class Outer,
          template <typename> class Inner>
const T &head(const nested<T, Outer, Inner> &xs) {
  assert(!xs.empty());
  return head(head(xs.values));
}
template <typename T, template <typename> class Outer,
          template <typename> class Inner>
const T &last(const nested<T, Outer, Inner> &xs) {
  assert(!xs.empty());
  return last(last(xs.values));
}

// Monad

template <template <typename> class C, typename T1,
          typename T = typename std::decay<T1>::type,
          std::enable_if_t<is_nested<C<T> >::value> * = nullptr>
auto munit(T1 &&x) {
  return C<T>{ std::forward<T1>(x) };
}

template <template <typename> class C, typename T, typename... As,
          std::enable_if_t<is_nested<C<T> >::value, C<T> > * = nullptr>
auto mmake(As &&... as) {
  return C<T>(typename C<T>::mmake(), std::forward<As>(as)...);
}

template <typename T, template <typename> class Outer,
          template <typename> class Inner, typename F, typename... As>
auto mbind(const nested<T, Outer, Inner> &xs, const F &f, const As &... as) {
  typedef cxx::invoke_of_t<F, T, As...> CR;
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

template <template <typename> class C, typename T,
          std::enable_if_t<is_nested<C<T> >::value> * = nullptr>
auto mzero() {
  return C<T>();
}

template <template <typename> class C, typename T, typename... Ts,
          std::enable_if_t<is_nested<C<T> >::value> * = nullptr>
auto msome(const T &x, const Ts &... xs) {
  static_assert(cxx::all<std::is_same<T, Ts>::value...>::value, "");
  return C<T>(msome<C<T>::template outer>(munit<C<T>::template inner>(x),
                                          munit<C<T>::template inner>(xs)...));
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
}

#define CXX_NESTED_HH_DONE
#else
#ifndef CXX_NESTED_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_NESTED_HH
