#ifndef FUN_FUN_IMPL_HPP
#define FUN_FUN_IMPL_HPP

#include "fun_decl.hpp"

#include <fun/maybe.hpp>

#include <cereal/types/tuple.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace fun {

// empty

template <typename CT, typename T> bool empty(const CT &xs) {
  struct f : std::tuple<> {
    bool operator()(const T &x) const { return false; }
  };
  struct op : std::tuple<> {
    bool operator()(bool x, bool y) const { return x && y; }
  };
  return foldMap(f(), op(), true, xs);
}

// size

template <typename CT, typename T> std::size_t size(const CT &xs) {
  struct f : std::tuple<> {
    std::size_t operator()(const T &x) const { return 1; }
  };
  struct op : std::tuple<> {
    std::size_t operator()(std::size_t x, std::size_t y) const { return x + y; }
  };
  return foldMap(f(), op(), 0, xs);
}

// convert

template <typename C1, typename C2T, typename T, typename C1T>
C1T convert(const C2T &xs) {
  struct f : std::tuple<> {
    auto operator()(const T &x) const { return munit<C1>(x); }
  };
  struct op : std::tuple<> {
    auto operator()(const C1T &x, const C1T &y) const { return mplus(x, y); }
  };
  return foldMap(f(), op(), mzero<C1, T>(), xs);
}
} // namespace fun

#define FUN_FUN_IMPL_HPP_DONE
#endif // #ifdef FUN_FUN_IMPL_HPP
#ifndef FUN_FUN_IMPL_HPP_DONE
#error "Cyclic include dependency"
#endif
