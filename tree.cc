#include "rpc.hh"

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_grid.hh"
#include "cxx_iota.hh"
#include "cxx_maybe.hh"
#include "cxx_monad.hh"
#include "cxx_nested.hh"
#include "cxx_shape.hh"
#include "cxx_tree.hh"
#define tree tree2
#define is_tree is_tree2
#include "cxx_tree2.hh"
#undef tree
#undef is_tree

#include <functional>
#include <memory>
#include <list>
#include <set>
#include <string>
#include <vector>

// Define templates one template argument
template <typename T> using either_ = cxx::either<std::string, T>;
template <typename T> using function_ = std::function<T(int)>;
template <typename T> using grid_ = cxx::grid<T, 2>;
template <typename T> using list_ = std::list<T>;
template <typename T> using set_ = std::set<T>;
template <typename T> using vector_ = std::vector<T>;

template <typename T> using nested_ = cxx::nested<T, vector_, std::shared_ptr>;
template <typename T> using nested2_ = cxx::nested<T, grid_, std::shared_ptr>;
template <typename T> using tree_ = cxx::tree<T, vector_, rpc::client>;
template <typename T> using tree2_ = cxx::tree2<T, nested2_>;

template <typename T> using boundaries_ = cxx::boundaries<T, 1>;
template <typename T> using boundaries2_ = cxx::boundaries<T, 2>;

int make_int(std::ptrdiff_t i) { return int(i); }
RPC_ACTION(make_int);

int add_int(int x, int y) { return x + y; }
RPC_ACTION(add_int);

double add_int_double(int x, int y) { return double(x + y); }
RPC_ACTION(add_int_double);

double make_double(int x) { return double(x); }
RPC_ACTION(make_double);

rpc::client<double> make_client_double(int x) {
  return rpc::make_client<double>(x);
}
RPC_ACTION(make_client_double);

double stencil_diff_double(int x, double bm, double bp) { return bp - bm; }
RPC_ACTION(stencil_diff_double);

double ghost_double(int x, bool face) { return double(x); }
RPC_ACTION(ghost_double);

int make_int2(const cxx::grid_region<2> &r, cxx::index<2> i) {
  return int(r.linear(i));
}
RPC_ACTION(make_int2);

int make_boundary2(int x, std::ptrdiff_t dir, bool face) {
  return !face ? -1.0 : +1.0;
}
RPC_ACTION(make_boundary2);

tree_<double> make_tree_double(int x) { return cxx::munit<tree_>(double(x)); }
RPC_ACTION(make_tree_double);

int rpc_main(int argc, char **argv) {

  // client (with functions)
  {
    auto i __attribute__((__unused__)) = cxx::iota<rpc::client>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);

    // auto i2 __attribute__((__unused__)) = cxx::iota<rpc::client>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary([](int x, std::ptrdiff_t dir,
    //                           bool face) { return !face ? -1.0 : +1.0; },
    //                        i2, dir, face);
    // });
    // auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
    //     [](int x, const auto &bs) {
    //       return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
    //     },
    //     [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2,
    //     b2);

    auto u __attribute__((__unused__)) = cxx::munit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<rpc::client>(double(x)); });
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<rpc::client>(cxx::munit<rpc::client>(1)));
  }

  // client (with actions)
  {
    auto i __attribute__((__unused__)) =
        cxx::iota<rpc::client>(make_int_action(), cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(add_int_action(), 0, i);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto f __attribute__((__unused__)) = cxx::fmap(make_double_action(), i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap(add_int_double_action(), i, 1);

    // auto i2 __attribute__((__unused__)) = cxx::iota<rpc::client>(
    //     make_int2_action(), cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary(make_boundary2_action(), i2, dir, face);
    // });
    // auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
    //     [](int x, const auto &bs) {
    //       return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
    //     },
    //     [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2,
    //     b2);

    auto u __attribute__((__unused__)) = cxx::munit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, make_client_double_action());
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<rpc::client>(cxx::munit<rpc::client>(1)));
  }

  // either
  {
    auto i __attribute__((__unused__)) = cxx::iota<either_>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);

    // auto i2 __attribute__((__unused__)) = cxx::iota<either_>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary([](int x, std::ptrdiff_t dir,
    //                           bool face) { return !face ? -1.0 : +1.0; },
    //                        i2, dir, face);
    // });

    auto u __attribute__((__unused__)) = cxx::munit<either_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<either_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<either_>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<either_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(m, u);
  }

  // function
  {
    auto u __attribute__((__unused__)) = cxx::munit<function_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<function_, int>(1);
    auto b __attribute__((__unused__)) = cxx::mbind(u, [](int x) {
      return std::function<double(int)>([](int x) { return double(x); });
    });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<function_>(cxx::munit<function_>(1)));
  }

  // grid
  {
    auto i2 __attribute__((__unused__)) = cxx::iota<grid_>(
        [](const cxx::grid_region<2> &r,
           const cxx::index<2> &i) { return int(r.linear(i)); },
        cxx::grid_region<2>(cxx::index<2>::set1(5)),
        cxx::grid_region<2>(cxx::index<2>::set1(5)));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i2);
    auto f = cxx::fmap([](int x) { return double(x); }, i2);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i2, 1);

    auto b2 = cxx::iota<boundaries2_>([&i2](std::ptrdiff_t dir, bool face) {
      return cxx::boundary([](int x, std::ptrdiff_t dir,
                              bool face) { return !face ? -1.0 : +1.0; },
                           i2, dir, face);
    });
    auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, const auto &bs) {
          return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
        },
        [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2, b2);

    // auto u __attribute__((__unused__)) = cxx::munit<grid_>(1);
    // auto m __attribute__((__unused__)) = cxx::mmake<grid_, int>(1);
    // auto b __attribute__((__unused__)) =
    //     cxx::mbind(u, [](int x) { return cxx::munit<grid_>(double(x)); });
    // auto z __attribute__((__unused__)) = cxx::mzero<grid_, int>();
    // auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // maybe
  {
    auto i __attribute__((__unused__)) = cxx::iota<cxx::maybe>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);

    // auto i2 __attribute__((__unused__)) = cxx::iota<cxx::maybe>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));

    auto u __attribute__((__unused__)) = cxx::munit<cxx::maybe>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<cxx::maybe, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<cxx::maybe>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<cxx::maybe, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(m, u);
  }

  // list
  {
    auto i __attribute__((__unused__)) = cxx::iota<list_>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(5));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto r __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, double bm, double bp) { return bp - bm; },
        [](int x, bool face) { return double(x); }, i, -1.0, +1.0);

    // auto i2 __attribute__((__unused__)) = cxx::iota<list_>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary([](int x, std::ptrdiff_t dir,
    //                           bool face) { return !face ? -1.0 : +1.0; },
    //                        i2, dir, face);
    // });
    // auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
    //     [](int x, const auto &bs) {
    //       return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
    //     },
    //     [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2,
    //     b2);

    auto u __attribute__((__unused__)) = cxx::munit<list_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<list_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<list_>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<list_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // nested<vector, shared_ptr>
  {
    auto i __attribute__((__unused__)) = cxx::iota<nested_>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(5));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto r __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, double bm, double bp) { return bp - bm; },
        [](int x, bool face) { return double(x); }, i, -1.0, +1.0);

    auto i2 __attribute__((__unused__)) = cxx::iota<nested_>(
        [](const cxx::grid_region<1> &r,
           const cxx::index<1> &i) { return int(r.linear(i)); },
        cxx::grid_region<1>(cxx::index<1>::set1(5)),
        cxx::grid_region<1>(cxx::index<1>::set1(5)));
    auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
      return cxx::boundary([](int x, std::ptrdiff_t dir,
                              bool face) { return !face ? -1.0 : +1.0; },
                           i2, dir, face);
    });
    auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, const auto &bs) { return bs(0, true) - bs(0, false); },
        [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2, b2);

    auto u __attribute__((__unused__)) = cxx::munit<nested_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<nested_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<nested_>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<nested_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // nested2<grid, shared_ptr>
  {
    auto i2 __attribute__((__unused__)) = cxx::iota<nested2_>(
        [](const cxx::grid_region<2> &r,
           const cxx::index<2> &i) { return int(r.linear(i)); },
        cxx::grid_region<2>(cxx::index<2>::set1(5)),
        cxx::grid_region<2>(cxx::index<2>::set1(5)));
    auto b2 = cxx::iota<boundaries2_>([&i2](std::ptrdiff_t dir, bool face) {
      return cxx::boundary([](int x, std::ptrdiff_t dir,
                              bool face) { return !face ? -1.0 : +1.0; },
                           i2, dir, face);
    });
    auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, const auto &bs) {
          return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
        },
        [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2, b2);

    // auto u __attribute__((__unused__)) = cxx::munit<nested2_>(1);
    // auto m __attribute__((__unused__)) = cxx::mmake<nested2_, int>(1);
    // auto b __attribute__((__unused__)) =
    //     cxx::mbind(u, [](int x) { return cxx::munit<nested2_>(double(x)); });
    // auto z __attribute__((__unused__)) = cxx::mzero<nested2_, int>();
    // auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // set
  {
    auto i __attribute__((__unused__)) = cxx::iota<set_>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(5));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);

    auto u __attribute__((__unused__)) = cxx::munit<set_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<set_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<set_>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<set_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // shared_future
  {
    auto i __attribute__((__unused__)) = cxx::iota<rpc::shared_future>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);

    // auto i2 __attribute__((__unused__)) = cxx::iota<rpc::shared_future>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary([](int x, std::ptrdiff_t dir,
    //                           bool face) { return !face ? -1.0 : +1.0; },
    //                        i2, dir, face);
    // });

    auto u __attribute__((__unused__)) = cxx::munit<rpc::shared_future>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::shared_future, int>(1);
    auto b __attribute__((__unused__)) = cxx::mbind(
        u, [](int x) { return cxx::munit<rpc::shared_future>(double(x)); });
  }

  // shared_ptr
  {
    auto i __attribute__((__unused__)) = cxx::iota<std::shared_ptr>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(1));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);

    // auto i2 __attribute__((__unused__)) = cxx::iota<std::shared_ptr>(
    //     [](const cxx::grid_region<2> &r,
    //        const cxx::index<2> &i) { return int(r.linear(i)); },
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)),
    //     cxx::grid_region<2>(cxx::index<2>::set1(1)));
    // auto b2 = cxx::iota<boundaries_>([&i2](std::ptrdiff_t dir, bool face) {
    //   return cxx::boundary([](int x, std::ptrdiff_t dir,
    //                           bool face) { return !face ? -1.0 : +1.0; },
    //                        i2, dir, face);
    // });

    auto u __attribute__((__unused__)) = cxx::munit<std::shared_ptr>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<std::shared_ptr, int>(1);
    auto b __attribute__((__unused__)) = cxx::mbind(
        u, [](int x) { return cxx::munit<std::shared_ptr>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<std::shared_ptr, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(m, u);
  }

  // tree<vector, client>
  {
    auto i __attribute__((__unused__)) =
        cxx::iota<tree_>(make_int_action(), cxx::iota_range_t(5));
    auto s __attribute__((__unused__)) = cxx::fold(add_int_action(), 0, i);
    auto f = cxx::fmap(make_double_action(), i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap(add_int_double_action(), i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto r __attribute__((__unused__)) = cxx::stencil_fmap(
        stencil_diff_double_action(), ghost_double_action(), i, -1.0, +1.0);

    auto u __attribute__((__unused__)) = cxx::munit<tree_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<tree_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, make_tree_double_action());
    auto z __attribute__((__unused__)) = cxx::mzero<tree_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // tree2<nested2> = tree<nested2<grid, shared_ptr> >
  {
    auto i2 __attribute__((__unused__)) = cxx::iota<tree2_>(
        [](const cxx::grid_region<2> &r,
           const cxx::index<2> &i) { return int(r.linear(i)); },
        cxx::grid_region<2>(cxx::index<2>::set1(5)),
        cxx::grid_region<2>(cxx::index<2>::set1(5)));
    auto b2 = cxx::iota<boundaries2_>([&i2](std::ptrdiff_t dir, bool face) {
      return cxx::boundary([](int x, std::ptrdiff_t dir,
                              bool face) { return !face ? -1.0 : +1.0; },
                           i2, dir, face);
    });
    auto r2 __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, const auto &bs) {
          return (bs(0, true) - bs(0, false)) + (bs(1, true) - bs(1, false));
        },
        [](int x, std::ptrdiff_t dir, bool face) { return double(x); }, i2, b2);

    // auto u __attribute__((__unused__)) = cxx::munit<tree2_>(1);
    // auto m __attribute__((__unused__)) = cxx::mmake<tree2_, int>(1);
    // auto b __attribute__((__unused__)) =
    //     cxx::mbind(u, [](int x) { return cxx::munit<tree2_>(double(x)); });
    // auto z __attribute__((__unused__)) = cxx::mzero<tree2_, int>();
    // auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  // vector
  {
    auto i __attribute__((__unused__)) = cxx::iota<vector_>(
        [](std::ptrdiff_t i) { return int(i); }, cxx::iota_range_t(5));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, i);
    auto f = cxx::fmap([](int x) { return double(x); }, i);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, i, 1);
    auto h __attribute__((__unused__)) = cxx::head(i);
    auto l __attribute__((__unused__)) = cxx::last(i);
    auto r __attribute__((__unused__)) = cxx::stencil_fmap(
        [](int x, double bm, double bp) { return bp - bm; },
        [](int x, bool face) { return double(x); }, i, -1.0, +1.0);

    auto u __attribute__((__unused__)) = cxx::munit<vector_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<vector_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<vector_>(double(x)); });
    auto z __attribute__((__unused__)) = cxx::mzero<vector_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
  }

  return 0;
}
