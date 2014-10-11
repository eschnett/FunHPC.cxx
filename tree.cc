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
    auto u __attribute__((__unused__)) = cxx::unit<function_>(1);
    auto m __attribute__((__unused__)) = cxx::make<function_, int>(1);
    auto b __attribute__((__unused__)) = cxx::bind(u, [](int x) {
      return std::function<double(int)>([](int x) { return double(x); });
    });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<function_>(cxx::unit<function_>(1)));
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<set_>(1);
    auto m __attribute__((__unused__)) = cxx::make<set_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<set_>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<set_>(cxx::unit<set_>(1)));
    auto z __attribute__((__unused__)) = cxx::zero<set_, int>();
    auto p __attribute__((__unused__)) = cxx::plus(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<std::shared_ptr>(1);
    auto m __attribute__((__unused__)) = cxx::make<std::shared_ptr, int>(1);
    auto b __attribute__((__unused__)) = cxx::bind(
        u, [](int x) { return cxx::unit<std::shared_ptr>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<std::shared_ptr>(cxx::unit<std::shared_ptr>(1)));
    auto z __attribute__((__unused__)) = cxx::zero<std::shared_ptr, int>();
    auto p __attribute__((__unused__)) = cxx::plus(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::make<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<rpc::client>(double(x)); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<rpc::client>(cxx::unit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::make<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, make_client_double_action());
    auto f __attribute__((__unused__)) = cxx::fmap(make_double_action(), u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap(add_int_double_action(), u, 1);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<rpc::client>(cxx::unit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(add_int_action(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<rpc::shared_future>(1);
    auto m __attribute__((__unused__)) = cxx::make<rpc::shared_future, int>(1);
    auto b __attribute__((__unused__)) = cxx::bind(
        u, [](int x) { return rpc::make_ready_future<double>(x).share(); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j0 __attribute__((__unused__)) =
        cxx::unit<rpc::shared_future>(cxx::unit<rpc::shared_future>(1))
            .unwrap();
    auto j __attribute__((__unused__)) = cxx::join(
        cxx::unit<rpc::shared_future>(cxx::unit<rpc::shared_future>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<vector_>(1);
    auto m __attribute__((__unused__)) = cxx::make<vector_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<vector_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto z __attribute__((__unused__)) = cxx::zero<vector_, int>();
    auto p __attribute__((__unused__)) = cxx::plus(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<either_>(1);
    auto m __attribute__((__unused__)) = cxx::make<either_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<either_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::zero<either_, int>();
    auto p __attribute__((__unused__)) = cxx::plus(m, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<cxx::maybe>(1);
    auto m __attribute__((__unused__)) = cxx::make<cxx::maybe, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<cxx::maybe>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::zero<cxx::maybe, int>();
    auto p __attribute__((__unused__)) = cxx::plus(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::unit<tree_>(1);
    auto m __attribute__((__unused__)) = cxx::make<tree_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::bind(u, [](int x) { return cxx::unit<tree_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) =
        cxx::join(cxx::unit<tree_>(cxx::unit<tree_>(1)));
    auto z __attribute__((__unused__)) = cxx::zero<tree_, int>();
    auto p __attribute__((__unused__)) = cxx::plus(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  return 0;
}
