#include "rpc.hh"

#include "cxx_foldable.hh"
#include "cxx_maybe.hh"
#include "cxx_monad.hh"
#include "cxx_tree.hh"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Define a set with only one template argument
template <typename T> using set_ = std::set<T>;

// Define a vector with only one template argument
template <typename T> using vector_ = std::vector<T>;

// Define an either with only one template argument
template <typename T> using either_ = cxx::either<std::string, T>;

// Define a tree with only one template argument
template <typename T> using tree_ = cxx::tree<T, vector_, std::shared_ptr>;

int rpc_main(int argc, char **argv) {

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<std::shared_ptr>(1);
    auto m __attribute__((__unused__)) =
        cxx::monad::make<std::shared_ptr, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<std::shared_ptr, double>(
            u, [](int x) { return std::make_shared<double>(x); });
    auto f __attribute__((__unused__)) =
        cxx::monad::fmap<std::shared_ptr, double>([](int x) {
                                                    return double(x);
                                                  },
                                                  u);
    auto f2 __attribute__((__unused__)) =
        cxx::monad::fmap<std::shared_ptr, double>([](int x, int y) {
                                                    return double(x + y);
                                                  },
                                                  u, 1);
    auto z __attribute__((__unused__)) =
        cxx::monad::zero<std::shared_ptr, int>();
    auto p __attribute__((__unused__)) =
        cxx::monad::plus<std::shared_ptr>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<set_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<set_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<set_, double>(
        u, [](int x) { return set_<double>{ double(x) }; });
    auto f __attribute__((__unused__)) =
        cxx::monad::fmap<set_, double>([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::monad::zero<set_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<set_>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) =
        cxx::monad::unit<rpc::shared_future>(1);
    auto m __attribute__((__unused__)) =
        cxx::monad::make<rpc::shared_future, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<rpc::shared_future, double>(
            u, [](int x) { return rpc::make_ready_future<double>(x).share(); });
    auto f __attribute__((__unused__)) =
        cxx::monad::fmap<rpc::shared_future, double>([](int x) {
                                                       return double(x);
                                                     },
                                                     u);
    auto f2 __attribute__((__unused__)) =
        cxx::monad::fmap<rpc::shared_future, double>([](int x, int y) {
                                                       return double(x + y);
                                                     },
                                                     u, 1);
    auto z __attribute__((__unused__)) =
        cxx::monad::zero<rpc::shared_future, int>();
    auto p __attribute__((__unused__)) =
        cxx::monad::plus<rpc::shared_future>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<vector_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<vector_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<vector_, double>(
        u, [](int x) { return vector_<double>(1, x); });
    auto f =
        cxx::monad::fmap<vector_, double>([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::monad::fmap<vector_, double>([](int x,
                                             int y) { return double(x + y); },
                                          u, 1);
    auto z __attribute__((__unused__)) = cxx::monad::zero<vector_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<vector_>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<either_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<either_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<either_, double>(
        u, [](int x) { return either_<double>(x); });
    auto f =
        cxx::monad::fmap<either_, double>([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::monad::zero<either_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<either_>(m, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<cxx::maybe>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<cxx::maybe, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<cxx::maybe, double>(
        u, [](int x) { return cxx::maybe<double>(x); });
    auto f =
        cxx::monad::fmap<cxx::maybe, double>([](int x) { return double(x); },
                                             u);
    auto z __attribute__((__unused__)) = cxx::monad::zero<cxx::maybe, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<cxx::maybe>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  {
    cxx::tree<double, vector_, std::shared_ptr> t;
    bool e __attribute__((__unused__)) = t.empty();
    size_t s __attribute__((__unused__)) = t.size();
    bool es __attribute__((__unused__)) = t.empty_slow();
    size_t ss __attribute__((__unused__)) = t.size_slow();

    auto r __attribute__((__unused__)) =
        t.foldl([](double s, double v) { return s + v; }, 0.0);

    auto ti __attribute__((__unused__)) =
        cxx::tree<int, vector_, std::shared_ptr>(
            cxx::tree<int, vector_, std::shared_ptr>::fmap(),
            [](double x) { return int(lrint(x)); }, t);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<tree_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<tree_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<tree_, double>(
        u, [](int x) { return cxx::monad::unit<tree_>(double(x)); });
    auto f =
        cxx::monad::fmap<tree_, double>([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) = cxx::monad::join<tree_>(
        cxx::monad::unit<tree_>(cxx::monad::unit<tree_>(1)));
    auto z __attribute__((__unused__)) = cxx::monad::zero<tree_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<tree_>(z, u);
    auto s __attribute__((__unused__)) =
        cxx::foldable::foldl(std::plus<int>(), 0, u);
  }

  return 0;
}
