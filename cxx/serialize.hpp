#ifndef CXX_SERIALIZE_HPP
#define CXX_SERIALIZE_HPP

#include <cereal/archives/binary.hpp>

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace cxx {
namespace detail {
template <typename T> void serialize_anchor_f() {}
static_assert(sizeof &serialize_anchor_f<void> <= sizeof(std::uintptr_t), "");
const std::uintptr_t serialize_anchor =
    std::uintptr_t(&serialize_anchor_f<void>);
}
}

namespace cereal {
// This exists only to throw a static_assert to let users know we
// don't support raw pointers
template <typename Archive, typename T>
std::enable_if_t<!std::is_function<T>::value, void> serialize(Archive &, T *&) {
  static_assert(cereal::traits::detail::delay_static_assert<T>::value,
                "Cereal does not support serializing raw pointers - please use "
                "a smart pointer");
}

// function pointers

// Note: We subtract an "anchor address" to handle ASLR (address space
// layout randomization)

template <typename Archive, typename F,
          std::enable_if_t<std::is_function<F>::value> * = nullptr>
void save(Archive &ar, F *const &f) {
  std::uintptr_t offset = std::uintptr_t(f) - cxx::detail::serialize_anchor;
  ar(offset);
}
template <class Archive, typename F,
          std::enable_if_t<std::is_function<F>::value> * = nullptr>
void load(Archive &ar, F *&f) {
  std::uintptr_t offset;
  ar(offset);
  f = (F *)(offset + cxx::detail::serialize_anchor);
}

// member function pointers

// Note: This follows the Itanium C++ ABI
// <http://refspecs.linuxbase.org/cxxabi-1.86.html>

template <
    typename Archive, typename F,
    std::enable_if_t<std::is_member_function_pointer<F>::value> * = nullptr>
void save(Archive &ar, F const &f) {
  struct {
    std::uintptr_t fptr;
    std::ptrdiff_t off;
  } buf;
  static_assert(sizeof f == sizeof buf, "");
  std::memcpy(&buf, &f, sizeof buf);
  if (!(buf.fptr & 1))
    buf.fptr -= cxx::detail::serialize_anchor;
  ar(buf.fptr, buf.off);
}
template <
    class Archive, typename F,
    std::enable_if_t<std::is_member_function_pointer<F>::value> * = nullptr>
void load(Archive &ar, F &f) {
  struct {
    std::uintptr_t fptr;
    std::ptrdiff_t off;
  } buf;
  static_assert(sizeof f == sizeof buf, "");
  ar(buf.fptr, buf.off);
  if (!(buf.fptr & 1))
    buf.fptr += cxx::detail::serialize_anchor;
  std::memcpy(&f, &buf, sizeof buf);
}

// member object pointers

template <typename Archive, typename T,
          std::enable_if_t<std::is_member_object_pointer<T>::value> * = nullptr>
void save(Archive &ar, T const &m) {
  std::ptrdiff_t buf;
  static_assert(sizeof m == sizeof buf, "");
  std::memcpy(&buf, &m, sizeof buf);
  ar(buf);
}
template <class Archive, typename T,
          std::enable_if_t<std::is_member_object_pointer<T>::value> * = nullptr>
void load(Archive &ar, T &m) {
  std::ptrdiff_t buf;
  static_assert(sizeof m == sizeof buf, "");
  ar(buf);
  std::memcpy(&m, &buf, sizeof buf);
}
}

#define CXX_SERIALIZE_HPP_DONE
#endif // #ifdef CXX_SERIALIZE_HPP
#ifndef CXX_SERIALIZE_HPP_DONE
#error "Cyclic include dependency"
#endif
