#ifndef ADT_EITHER_HPP
#define ADT_EITHER_HPP

#include <cxx/invoke.hpp>

#include <cereal/access.hpp>

#include <cassert>
#include <type_traits>
#include <utility>

namespace adt {

template <typename L, typename R> class either;

template <typename L, typename R, typename... Args>
either<L, R> make_left(Args &&... args);
template <typename L, typename R, typename... Args>
either<L, R> make_right(Args &&... args);

template <typename L, typename R> class either {
  template <typename L1, typename R1, typename... Args>
  friend either<L1, R1> make_left(Args &&... args);
  template <typename L1, typename R1, typename... Args>
  friend either<L1, R1> make_right(Args &&... args);

  bool is_left;
  union {
    L left_;
    R right_;
  };
  void destruct() {
    if (is_left)
      left_.~L();
    else
      right_.~R();
  }

  friend class cereal::access;
  template <typename Archive> void save(Archive &ar) const {
    ar(is_left);
    if (is_left)
      ar(left_);
    else
      ar(right_);
  }
  template <typename Archive> void load(Archive &ar) {
    destruct();
    ar(is_left);
    if (is_left) {
      new (&left_) L;
      ar(left_);
    } else {
      new (&right_) R;
      ar(right_);
    }
  }

public:
  typedef L left_type;
  typedef R right_type;

  either() : is_left(true), left_() {}
  either(const either &other) {
    is_left = other.is_left;
    if (is_left)
      new (&left_) L(other.left_);
    else
      new (&right_) R(other.right_);
  }
  either(either &&other) {
    is_left = other.is_left;
    if (is_left)
      new (&left_) L(std::move(other.left_));
    else
      new (&right_) R(std::move(other.right_));
  }
  either &operator=(const either &other) {
    if (this != &other) {
      destruct();
      is_left = other.is_left;
      if (is_left)
        new (&left_) L(other.left_);
      else
        new (&right_) R(other.right_);
    }
    return *this;
  }
  either &operator=(either &&other) {
    destruct();
    is_left = other.is_left;
    if (is_left)
      new (&left_) L(std::move(other.left_));
    else
      new (&right_) R(std::move(other.right_));
    return *this;
  }
  ~either() { destruct(); }
  void swap(either &other) {
    using std::swap;
    if (is_left && other.is_left) {
      swap(left_, other.left_);
    } else if (!is_left && !other.is_left) {
      swap(right_, other.right_);
    } else {
      either tmp(std::move(*this));
      *this = std::move(other);
      other = std::move(tmp);
    }
  }

  template <typename... Args> static either<L, R> make_left(Args &&... args) {
    either<L, R> res;
    res.destruct();
    res.is_left = true;
    new (&res.left_) L(std::forward<Args>(args)...);
    return res;
  }
  template <typename... Args> static either<L, R> make_right(Args &&... args) {
    either<L, R> res;
    res.destruct();
    res.is_left = false;
    new (&res.right_) R(std::forward<Args>(args)...);
    return res;
  }

  bool left() const noexcept { return is_left; }
  bool right() const noexcept { return !is_left; }
  const L &get_left() const {
    assert(is_left);
    return left_;
  }
  L &get_left() {
    assert(is_left);
    return left_;
  }
  const R &get_right() const {
    assert(!is_left);
    return right_;
  }
  R &get_right() {
    assert(!is_left);
    return right_;
  }

  bool operator==(const either &other) const {
    if (left() != other.left())
      return false;
    return left() ? get_left() == other.get_left()
                  : get_right() == other.get_right();
  }
  bool operator!=(const either &other) const { return !(*this == other); }
  bool operator<(const either &other) const {
    if (left() && other.right())
      return true;
    if (right() && other.left())
      return false;
    return left() ? get_left() < other.get_left()
                  : get_right() < other.get_right();
  }
  bool operator>(const either &other) const { return other < *this; }
  bool operator<=(const either &other) const { return !(*this > other); }
  bool operator>=(const either &other) const { return !(*this < other); }

  template <typename FL, typename FR, typename... Args>
  auto from_either(FL &&fl, FR &&fr, Args &&... args) const {
    static_assert(std::is_same<cxx::invoke_of_t<FL, L, Args...>,
                               cxx::invoke_of_t<FR, R, Args...>>::value,
                  "");
    return left() ? cxx::invoke(std::forward<FL>(fl), get_left(),
                                std::forward<Args>(args)...)
                  : cxx::invoke(std::forward<FR>(fr), get_right(),
                                std::forward<Args>(args)...);
  }

  template <typename FL, typename FR, typename... Args>
  auto transform(FL &&fl, FR &&fr, Args &&... args) const {
    typedef cxx::invoke_of_t<FL, L, Args...> RL;
    typedef cxx::invoke_of_t<FR, R, Args...> RR;
    return left()
               ? make_left<RL, RR>(cxx::invoke(std::forward<FL>(fl), get_left(),
                                               std::forward<Args>(args)...))
               : make_right<RL, RR>(cxx::invoke(std::forward<FR>(fr),
                                                get_right(),
                                                std::forward<Args>(args)...));
  }

  auto flip() const {
    return left() ? make_right<R, L>(get_left()) : make_left<R, L>(get_right());
  }
};
template <typename L, typename R> void swap(either<L, R> &x, either<L, R> &y) {
  x.swap(y);
}

template <typename L, typename R, typename... Args>
either<L, R> make_left(Args &&... args) {
  return either<L, R>::make_left(std::forward<Args>(args)...);
}
template <typename L, typename R, typename... Args>
either<L, R> make_right(Args &&... args) {
  return either<L, R>::make_right(std::forward<Args>(args)...);
}
}

#define ADT_EITHER_HPP_DONE
#endif // #ifdef ADT_EITHER_HPP
#ifndef ADT_EITHER_HPP_DONE
#error "Cyclic include dependency"
#endif
