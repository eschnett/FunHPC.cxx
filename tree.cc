#include "rpc.hh"

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_maybe.hh"
#include "cxx_monad.hh"

#include "cxx_tree.hh"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Define an either with only one template argument
template <typename T> using either_ = cxx::either<std::string, T>;

// Define a function with only one template argument
template <typename T> using function_ = std::function<T(int)>;

// Define a set with only one template argument
template <typename T> using set_ = std::set<T>;

// Define a vector with only one template argument
template <typename T> using vector_ = std::vector<T>;

// Define a tree with only one template argument
template <typename T> using tree_ = cxx::tree<T, vector_, std::shared_ptr>;

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

int rpc_main(int argc, char **argv) {

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

  {
    auto u __attribute__((__unused__)) = cxx::munit<set_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<set_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<set_>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<set_>(cxx::munit<set_>(1)));
    auto z __attribute__((__unused__)) = cxx::mzero<set_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<std::shared_ptr>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<std::shared_ptr, int>(1);
    auto b __attribute__((__unused__)) = cxx::mbind(
        u, [](int x) { return cxx::munit<std::shared_ptr>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<std::shared_ptr>(cxx::munit<std::shared_ptr>(1)));
    auto z __attribute__((__unused__)) = cxx::mzero<std::shared_ptr, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<rpc::client>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<rpc::client>(cxx::munit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, make_client_double_action());
    auto f __attribute__((__unused__)) = cxx::fmap(make_double_action(), u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap(add_int_double_action(), u, 1);
    auto j __attribute__((__unused__)) =
        cxx::mjoin(cxx::munit<rpc::client>(cxx::munit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::fold(add_int_action(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<rpc::shared_future>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<rpc::shared_future, int>(1);
    auto b __attribute__((__unused__)) = cxx::mbind(
        u, [](int x) { return rpc::make_ready_future<double>(x).share(); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j0 __attribute__((__unused__)) =
        cxx::munit<rpc::shared_future>(cxx::munit<rpc::shared_future>(1))
            .unwrap();
    auto j __attribute__((__unused__)) = cxx::mjoin(
        cxx::munit<rpc::shared_future>(cxx::munit<rpc::shared_future>(1)));
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<vector_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<vector_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<vector_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto z __attribute__((__unused__)) = cxx::mzero<vector_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<either_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<either_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<either_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::mzero<either_, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(m, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<cxx::maybe>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<cxx::maybe, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::mbind(u, [](int x) { return cxx::munit<cxx::maybe>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::mzero<cxx::maybe, int>();
    auto p __attribute__((__unused__)) = cxx::mplus(z, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::munit<tree_>(1);
    auto m __attribute__((__unused__)) = cxx::mmake<tree_, int>(1);
    // auto b __attribute__((__unused__)) =
    //     cxx::mbind(u, [](int x) { return cxx::munit<tree_>(double(x)); });
    // auto f = cxx::fmap([](int x) { return double(x); }, u);
    // auto j __attribute__((__unused__)) =
    //     cxx::mjoin(cxx::munit<tree_>(cxx::munit<tree_>(1)));
    auto z __attribute__((__unused__)) = cxx::mzero<tree_, int>();
    // auto p __attribute__((__unused__)) = cxx::mplus(z, u);
    auto s __attribute__((__unused__)) = cxx::fold(std::plus<int>(), 0, u);
  }

  return 0;
}
