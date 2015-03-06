#ifndef FUN_FUN_HPP
#define FUN_FUN_HPP

#include <cereal/types/tuple.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <tuple>

namespace fun {

// empty

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
bool empty(const CT &xs) {
  struct f : std::tuple<> {
    bool operator()(const T &x) const { return false; }
  };
  struct op : std::tuple<> {
    bool operator()(bool x, bool y) const { return x && y; }
  };
  return foldMap(f(), op(), true, xs);
}

// size

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
std::size_t size(const CT &xs) {
  struct f : std::tuple<> {
    std::size_t operator()(const T &x) const { return 1; }
  };
  struct op : std::tuple<> {
    std::size_t operator()(std::size_t x, std::size_t y) const { return x + y; }
  };
  return foldMap(f(), op(), 0, xs);
}

// convert

template <template <typename> class C1, typename C2T,
          template <typename> class C2 = fun_traits<C2T>::template constructor,
          typename T = typename fun_traits<C2T>::value_type>
C1<T> convert(const C2T &xs) {
  struct f : std::tuple<> {
    auto operator()(const T &x) const { return munit<C1>(x); }
  };
  struct op : std::tuple<> {
    auto operator()(const C1<T> &x, const C1<T> &y) const {
      return mplus(x, y);
    }
  };
  return foldMap(f(), op(), mzero<C1, T>(), xs);
}

// to_string

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
std::string to_string(const CT &xs) {
  struct f : std::tuple<> {
    auto operator()(const T &x) const {
      std::ostringstream os;
      os << x;
      return os.str();
    }
  };
  struct op : std::tuple<> {
    auto operator()(const std::string &x, const std::string &y) const {
      if (x.empty())
        return y;
      if (y.empty())
        return x;
      return x + ", " + y;
    }
  };
  return std::string("[") + foldMap(f(), op(), std::string(), xs) + "]";
}

// to_string_function

// namespace detail {
// template <typename T> using string_function = std::function<std::string(T)>;
// }
//
// template <typename CT,
//           template <typename> class C = fun_traits<CT>::template constructor,
//           typename T = typename fun_traits<CT>::value_type>
// std::function<std::string(T)> to_string_function(const CT &xs) {
//   return foldMap([](const auto &x) {
//                    return
//                    munit<detail::string_function>(detail::to_string(x));
//                  },
//                  [](const auto &x, const auto &y) { return mbind(x, y); },
//                  munit<detail::string_function>(std::string()));
// }
}

#define FUN_FUN_HPP_DONE
#endif // #ifdef FUN_FUN_HPP
#ifndef FUN_FUN_HPP_DONE
#error "Cyclic include dependency"
#endif
