#ifndef FUN_FUN_DECL_HPP
#define FUN_FUN_DECL_HPP

#include <cereal/types/tuple.hpp>

#include <functional>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace fun {

template <typename> struct fun_traits;

// empty

template <typename CT, typename T = typename fun::fun_traits<CT>::value_type>
bool empty(const CT &xs);

// size

template <typename CT, typename T = typename fun::fun_traits<CT>::value_type>
std::size_t size(const CT &xs);

// convert

template <typename C1, typename C2T,
          typename T = typename fun::fun_traits<C2T>::value_type,
          typename C1T = typename fun_traits<C1>::template constructor<T>>
C1T convert(const C2T &xs);

// An ostreamer is function that outputs something. In particular,
// there is an efficient way of combining ostreamers -- somthing that
// is not possible with regular ostreams.

class ostreamer {
  std::function<void(std::ostream &)> impl;
  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const {
    std::ostringstream os;
    os << *this;
    ar(std::move(os).str());
  }
  template <typename Archive> void load(Archive &ar) {
    std::string str;
    ar(str);
    impl = [str = std::move(str)](std::ostream & os) { os << str; };
  }

public:
  ostreamer() = default;
  ostreamer(const ostreamer &) = default;
  ostreamer(ostreamer &&) = default;
  ostreamer &operator=(const ostreamer &) = default;
  ostreamer &operator=(ostreamer &&) = default;
  void swap(ostreamer &other) {
    using std::swap;
    swap(impl, other.impl);
  }

  ostreamer(const std::string &str) {
    impl = [str](std::ostream &os) { os << str; };
  }
  ostreamer(std::string &&str) {
    impl = [str = std::move(str)](std::ostream & os) { os << str; };
  }
  template <typename T> ostreamer(T &&x) {
    std::ostringstream os;
    os.precision(std::numeric_limits<std::decay_t<T>>::max_digits10);
    os << std::forward<T>(x);
    auto str = std::move(os).str();
    impl = [str = std::move(str)](std::ostream & os) { os << str; };
  }

  ostreamer &operator+=(const ostreamer &other) {
    ostreamer self;
    swap(self);
    impl = [ self = std::move(self), other ](std::ostream & os) {
      os << self << other;
    };
    return *this;
  }
  ostreamer &operator+=(ostreamer &&other) {
    ostreamer self;
    swap(self);
    impl = [ self = std::move(self), other = std::move(other) ](std::ostream &
                                                                os) {
      os << self << other;
    };
    return *this;
  }

  template <typename T> ostreamer &operator<<(const ostreamer &ostr) {
    return *this += ostr;
  }
  template <typename T> ostreamer &operator<<(ostreamer &&ostr) {
    return *this += std::move(ostr);
  }
  template <typename T> ostreamer &operator<<(T &&x) {
    return *this += ostreamer(std::forward<T>(x));
  }

  friend std::ostream &operator<<(std::ostream &os, const ostreamer &ostr) {
    if (ostr.impl)
      ostr.impl(os);
    return os;
  }
};
inline void swap(ostreamer &left, ostreamer &right) { left.swap(right); }
inline ostreamer operator+(const ostreamer &left, const ostreamer &right) {
  return ostreamer(left) += right;
}
inline ostreamer operator+(const ostreamer &left, ostreamer &&right) {
  return ostreamer(left) += std::move(right);
}
inline ostreamer operator+(ostreamer &&left, const ostreamer &right) {
  return ostreamer(std::move(left)) += right;
}
inline ostreamer operator+(ostreamer &&left, ostreamer &&right) {
  return ostreamer(std::move(left)) += std::move(right);
}

template <typename T> ostreamer make_ostreamer(T &&x) {
  return {std::forward<T>(x)};
}
struct combine_ostreamers : std::tuple<> {
  ostreamer operator()(const ostreamer &left, const ostreamer &right) const {
    return left + right;
  }
  ostreamer operator()(const ostreamer &left, ostreamer &&right) const {
    return left + std::move(right);
  }
  ostreamer operator()(ostreamer &&left, const ostreamer &right) const {
    return std::move(left) + right;
  }
  ostreamer operator()(ostreamer &&left, ostreamer &&right) const {
    return std::move(left) + std::move(right);
  }
};

// to_ostreamer

template <typename CT, typename T = typename fun_traits<CT>::value_type>
ostreamer to_ostreamer(const CT &xs) {
  struct with_comma : std::tuple<> {
    auto operator()(const T &x) const {
      return make_ostreamer(x) + make_ostreamer(",");
    }
    auto operator()(T &&x) const {
      return make_ostreamer(std::move(x)) + make_ostreamer(",");
    }
  };
  return make_ostreamer("[") +
         foldMap(with_comma(), combine_ostreamers(), ostreamer(), xs) +
         make_ostreamer("]");
}
}

namespace std {
// operator<<

template <typename CT, typename T = typename fun::fun_traits<CT>::value_type>
std::ostream &operator<<(std::ostream &os, const CT &xs) {
  return os << fun::to_ostreamer(xs);
}

// to_string

template <typename CT, typename T = typename fun::fun_traits<CT>::value_type>
std::string to_string(const CT &xs) {
  std::ostringstream os;
  os << xs;
  return std::move(os).str();
}
}

#define FUN_FUN_DECL_HPP_DONE
#endif // #ifdef FUN_FUN_DECL_HPP
#ifndef FUN_FUN_DECL_HPP_DONE
#error "Cyclic include dependency"
#endif
