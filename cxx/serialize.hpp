#ifndef CXX_SERIALIZE_HPP
#define CXX_SERIALIZE_HPP

#include <cxx/cassert.hpp>

#include <cereal/archives/binary.hpp>

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace cereal {

namespace detail {
void serialize_anchor_f();
inline std::uintptr_t serialize_anchor() {
  return std::uintptr_t(&serialize_anchor_f);
}
}

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
  std::uintptr_t offset =
      bool(f) ? std::uintptr_t(f) - detail::serialize_anchor() : 0;
  ar(offset);
}
template <class Archive, typename F,
          std::enable_if_t<std::is_function<F>::value> * = nullptr>
void load(Archive &ar, F *&f) {
  std::uintptr_t offset;
  ar(offset);
  f = offset ? (F *)(offset + detail::serialize_anchor()) : nullptr;
}

// member function pointers

// Note: This follows the System V ADM64 ABI
// <http://refspecs.linuxfoundation.org/elf/x86_64-abi-0.95.pdf> and
// the Itanium C++ ABI
// <http://refspecs.linuxbase.org/cxxabi-1.86.html>

template <
    typename Archive, typename F,
    std::enable_if_t<std::is_member_function_pointer<F>::value> * = nullptr>
void save(Archive &ar, F const &f) {
  struct {
    std::uintptr_t fptr; // function pointer, or virtual table offset + 1
    std::ptrdiff_t adj;
  } buf;
  static_assert(sizeof f == sizeof buf, "");
  std::memcpy(&buf, &f, sizeof buf);
  if ((buf.fptr & 1) == 0) {
    if (buf.fptr != 0) {
      buf.fptr -= detail::serialize_anchor();
      cxx_assert(buf.fptr != 0);
    }
    cxx_assert((buf.fptr & 1) == 0);
  }
  ar(buf.fptr, buf.adj);
}
template <
    class Archive, typename F,
    std::enable_if_t<std::is_member_function_pointer<F>::value> * = nullptr>
void load(Archive &ar, F &f) {
  struct {
    std::uintptr_t fptr;
    std::ptrdiff_t adj;
  } buf;
  static_assert(sizeof f == sizeof buf, "");
  ar(buf.fptr, buf.adj);
  if ((buf.fptr & 1) == 0) {
    if (buf.fptr != 0) {
      buf.fptr += detail::serialize_anchor();
      cxx_assert(buf.fptr != 0);
    }
    cxx_assert((buf.fptr & 1) == 0);
  }
  std::memcpy(&f, &buf, sizeof f);
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

#if 0
namespace cxx {
// captureless lambdas

// It is not possible to create a captureless lambda via a default
// constructor, although this "should" work. We don't know how to
// cheat and let Cereal use an uninitialized object, which also
// "should" work. Thus we convert captureless lambdas to a function
// pointer before serializing it.

namespace detail {
template <typename R, typename... Args>
auto get_funptr(R (*fptr)(Args...)) -> R (*)(Args...) {
  return fptr;
}
}

template <typename T, typename R, typename... Args>
auto lambda_to_function(const T &lambda) {
  return detail::get_funptr(lambda);
  // return detail::get_funptr<R, Args...>(lambda);
  // return (R (*)(Args...))lambda;
}
}
#endif

#define CXX_SERIALIZE_HPP_DONE
#endif // #ifdef CXX_SERIALIZE_HPP
#ifndef CXX_SERIALIZE_HPP_DONE
#error "Cyclic include dependency"
#endif
