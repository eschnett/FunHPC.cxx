#ifndef FUN_FUN_HPP
#define FUN_FUN_HPP

#include <functional>
#include <sstream>
#include <string>

namespace fun {

// empty

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
bool empty(const CT &xs) {
  return foldMap([](const auto &) { return false; },
                 [](const auto &x, const auto &y) { return x && y; }, true, xs);
}

// size

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
std::size_t size(const CT &xs) {
  return foldMap([](const auto &) { return std::size_t(1); },
                 [](const auto &x, const auto &y) { return x + y; }, 0, xs);
}

// convert

template <template <typename> class C1, typename C2T,
          template <typename> class C2 = fun_traits<C2T>::template constructor,
          typename T = typename fun_traits<C2T>::value_type>
C1<T> convert(const C2T &xs) {
  return foldMap([](const auto &x) { return munit<C1>(x); },
                 [](const auto &x, const auto &y) { return mplus(x, y); },
                 mzero<C1, T>(), xs);
}

// to_string

namespace detail {
template <typename T> std::string to_string(const T &x) {
  std::ostringstream os;
  os << x;
  return os.str();
}
}

template <typename CT,
          template <typename> class C = fun_traits<CT>::template constructor,
          typename T = typename fun_traits<CT>::value_type>
std::string to_string(const CT &xs) {
  return std::string("[") +
         foldMap([](const auto &x) { return detail::to_string(x); },
                 [](const auto &x, const auto &y) {
                   if (x.empty())
                     return y;
                   if (y.empty())
                     return x;
                   return x + ", " + y;
                 },
                 std::string(), xs) +
         "]";
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
