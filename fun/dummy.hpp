#ifndef FUN_DUMMY_HPP
#define FUN_DUMMY_HPP

#include <adt/dummy.hpp>

#include <cxx/invoke.hpp>
#include <cxx/type_traits.hpp>
#include <fun/fun_decl.hpp>

#include <type_traits>

namespace fun {

// is_dummy

namespace detail {
template <typename> struct is_dummy : std::false_type {};
template <> struct is_dummy<adt::dummy> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <> struct fun_traits<adt::dummy> {
  template <typename U> using constructor = adt::dummy;
  typedef adt::dummy dummy;
  typedef adt::dummy value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;
  typedef adt::dummy boundary_dummy;

  static constexpr std::size_t min_size() { return 0; }
  static constexpr std::size_t max_size() { return 0; }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_dummy<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F &&, std::ptrdiff_t, Args &&...>>
adt::dummy iotaMap(const F &f, const adt::irange_t &inds, const Args &... args);

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_dummy<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F &&, adt::index_t<D>, Args &&...>>
adt::dummy iotaMapMulti(F &&f, const adt::steprange_t<D> &inds,
                        Args &&... args);

// fmap

template <typename F, typename... Args, typename CT = adt::dummy,
          typename R = cxx::invoke_of_t<F &&, adt::dummy, Args &&...>>
adt::dummy fmap(F &&f, adt::dummy xs, Args &&... args);

template <
    typename F, typename... Args, typename CT = adt::dummy,
    typename R = cxx::invoke_of_t<F &&, adt::dummy, adt::dummy, Args &&...>>
adt::dummy fmap2(F &&f, adt::dummy xs, const adt::dummy &ys, Args &&... args);

// fmapStencil

template <
    typename F, typename G, typename B, typename... Args,
    typename B_ = cxx::invoke_of_t<G &&, adt::dummy, std::ptrdiff_t>,
    typename R = cxx::invoke_of_t<F &&, adt::dummy, std::size_t, B, B, Args...>>
adt::dummy fmapStencil(F &&f, G &&g, adt::dummy xs, std::size_t bmask,
                       const B &bm, const B &bp, Args &&... args);

template <std::size_t D, typename F, typename G, typename... Args,
          std::enable_if_t<D == 1> * = nullptr,
          typename B = cxx::invoke_of_t<G &&, adt::dummy, std::ptrdiff_t>,
          typename R =
              cxx::invoke_of_t<F &&, adt::dummy, std::size_t, B, B, Args &&...>>
adt::dummy fmapStencilMulti(F &&f, G &&g, adt::dummy xs, std::size_t bmask,
                            adt::dummy bm0, adt::dummy bp1, Args &&... args);

template <std::size_t D, typename F, typename G, typename... Args,
          std::enable_if_t<D == 2> * = nullptr,
          typename B = cxx::invoke_of_t<G &&, adt::dummy, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F &&, adt::dummy, std::size_t, B, B, B,
                                        B, Args &&...>>
adt::dummy fmapStencilMulti(F &&f, G &&g, adt::dummy xs, std::size_t bmask,
                            adt::dummy bm0, adt::dummy bm1, adt::dummy bp0,
                            adt::dummy bp1, Args &&... args);

// head, last

template <typename = void> adt::dummy head(adt::dummy xs);
template <typename = void> adt::dummy last(adt::dummy xs);

// boundary

template <typename = void>
adt::dummy boundary(const adt::dummy &xs, std::ptrdiff_t i);

// boundaryMap

template <
    typename F, typename... Args,
    typename R = cxx::invoke_of_t<F &&, adt::dummy, std::ptrdiff_t, Args &&...>>
adt::dummy boundaryMap(F &&f, const adt::dummy &xs, std::ptrdiff_t i,
                       Args &&... args);

// indexing

template <typename = void> adt::dummy getIndex(adt::dummy xs, std::ptrdiff_t i);

template <typename> class accumulator;
template <> class accumulator<adt::dummy> {
public:
  accumulator(std::ptrdiff_t n);
  adt::dummy &operator[](std::ptrdiff_t i);
  adt::dummy finalize();
};

// foldMap

template <typename F, typename Op, typename Z, typename... Args,
          typename R = cxx::invoke_of_t<F &&, adt::dummy, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, adt::dummy xs, Args &&... args);

template <
    typename F, typename Op, typename Z, typename... Args,
    typename R = cxx::invoke_of_t<F &&, adt::dummy, adt::dummy, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, adt::dummy xs, adt::dummy ys,
           Args &&... args);

// dump

template <typename = void> ostreamer dump(adt::dummy xs) {
  return ostreamer("dummy{}");
}

// munit

template <typename C, std::enable_if_t<detail::is_dummy<C>::value> * = nullptr>
adt::dummy munit(adt::dummy x);

// mjoin

template <typename = void> adt::dummy mjoin(adt::dummy xss);

// mbind

template <
    typename F, typename... Args,
    typename CR = std::decay_t<cxx::invoke_of_t<F &&, adt::dummy, Args &&...>>>
CR mbind(F &&f, adt::dummy xs, Args &&... args);

// mextract

template <typename = void> adt::dummy mextract(adt::dummy xs);

// mfoldMap

template <typename F, typename Op, typename Z, typename... Args,
          typename R = cxx::invoke_of_t<F &&, adt::dummy, Args &&...>>
adt::dummy mfoldMap(F &&f, Op &&op, Z &&z, adt::dummy xs, Args &&... args);

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_dummy<C>::value> * = nullptr>
adt::dummy mzero();

// mplus

template <typename... Ts> adt::dummy mplus(adt::dummy xs, Ts... yss);

// msome

template <typename C, typename... Ts,
          std::enable_if_t<detail::is_dummy<C>::value> * = nullptr>
adt::dummy msome(adt::dummy x, Ts... ys);

// mempty

template <typename = void> bool mempty(adt::dummy xs);

// msize

template <typename = void> std::size_t msize(adt::dummy xs);
} // namespace fun

#define FUN_DUMMY_HPP_DONE
#endif // #ifdef FUN_DUMMY_HPP
#ifndef FUN_DUMMY_HPP_DONE
#error "Cyclic include dependency"
#endif
