#ifndef ADT_MAXARRAY_HPP
#define ADT_MAXARRAY_HPP

#include <cxx/cassert.hpp>

#include <cereal/access.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace adt {

template <typename T, std::size_t N> struct maxarray {
  std::array<T, N> elts;
  std::size_t used;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(elts, used); }

public:
  typedef T element_type;

  maxarray() : used(0) {}
  explicit maxarray(std::size_t n) : used(n) { cxx_assert(n <= N); }
  void swap(maxarray &other) {
    using std::swap;
    swap(used, other.used);
    swap(elts, other.elts);
  }

  bool invariant() const noexcept { return used <= N; }

  void resize(std::size_t n) {
    cxx_assert(n <= N);
    used = n;
  }
  void reset() noexcept { used = 0; }

  constexpr bool empty() const noexcept { return used == 0; }
  constexpr std::size_t size() const noexcept { return used; }
  constexpr std::size_t max_size() const noexcept { return N; }

  void push_back(const T &value) {
    cxx_assert(used < N);
    *end() = value;
    ++used;
  }
  void push_back(T &&value) {
    cxx_assert(used < N);
    *end() = std::move(value);
    ++used;
  }

  T *data() noexcept { return elts.data(); }
  const T *data() const noexcept { return elts.data(); }

  const T &operator[](std::size_t i) const {
    cxx_assert(i < N);
    cxx_assert(std::ptrdiff_t(i) >= 0);
    return elts[i];
  }
  constexpr T &operator[](std::size_t i) {
    cxx_assert(i < N);
    cxx_assert(std::ptrdiff_t(i) >= 0);
    return elts[i];
  }

  constexpr T *begin() noexcept { return data(); }
  constexpr const T *begin() const noexcept { return data(); }
  constexpr T *end() noexcept { return data() + size(); }
  constexpr const T *end() const noexcept { return data() + size(); }

  template <std::size_t N2>
  bool operator==(const maxarray<T, N2> &other) const {
    return used == other.used &&
           std::equal(begin(), end(), other.begin(), other.end());
  }
  template <std::size_t N2>
  bool operator!=(const maxarray<T, N2> &other) const {
    return !(*this == other);
  }
  template <std::size_t N2> bool operator<(const maxarray<T, N2> &other) const {
    return std::lexicographical_compare(begin(), end(), other.begin(),
                                        other.end());
  }
  template <std::size_t N2> bool operator>(const maxarray<T, N2> &other) const {
    return other < *this;
  }
  template <std::size_t N2>
  bool operator<=(const maxarray<T, N2> &other) const {
    return !(*this > other);
  }
  template <std::size_t N2>
  bool operator>=(const maxarray<T, N2> &other) const {
    return !(*this < other);
  }
};
template <typename T, std::size_t N>
void swap(maxarray<T, N> &x, maxarray<T, N> &y) noexcept {
  x.swap(y);
}
}

#define ADT_MAXARRAY_HPP_DONE
#endif // #ifdef ADT_MAXARRAY_HPP
#ifndef ADT_MAXARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
