#ifndef CXX_KINDS_HH
#define CXX_KINDS_HH

#include <array>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

namespace cxx {

template <typename> struct kinds;

// array
template <typename T, size_t N> struct kinds<std::array<T, N> > {
  typedef T element_type;
  template <typename U> using constructor = std::array<U, N>;
};
template <typename T> struct is_array : std::false_type {};
template <typename T, size_t N>
struct is_array<std::array<T, N> > : std::true_type {};

// function
template <typename T, typename A> struct kinds<std::function<T(A)> > {
  typedef T element_type;
  template <typename U> using constructor = std::function<U(A)>;
};
template <typename T> struct is_function : std::false_type {};
template <typename T, typename A>
struct is_function<std::function<T(A)> > : std::true_type {};

// list
template <typename T, typename Allocator>
struct kinds<std::list<T, Allocator> > {
  typedef T element_type;
  template <typename U>
  using constructor =
      std::list<U /*TODO typename Allocator::template rebind<U>::other*/>;
};
template <typename T> struct is_list : std::false_type {};
template <typename T, typename Allocator>
struct is_list<std::list<T, Allocator> > : std::true_type {};

// set
template <typename T, typename Compare, typename Allocator>
struct kinds<std::set<T, Compare, Allocator> > {
  typedef T element_type;
  template <typename U>
  using constructor = std::set<
      U /*TODO std::less<U>, typename Allocator::template rebind<U>::other*/>;
};
template <typename T> struct is_set : std::false_type {};
template <typename T, typename Compare, typename Allocator>
struct is_set<std::set<T, Compare, Allocator> > : std::true_type {};

// shared_ptr
template <typename T> struct kinds<std::shared_ptr<T> > {
  typedef T element_type;
  template <typename U> using constructor = std::shared_ptr<U>;
};
template <typename T> struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T> > : std::true_type {};

// vector
template <typename T, typename Allocator>
struct kinds<std::vector<T, Allocator> > {
  typedef T element_type;
  template <typename U>
  using constructor =
      std::vector<U /*TODO typename Allocator::template rebind<U>::other*/>;
};
template <typename T> struct is_vector : std::false_type {};
template <typename T, typename Allocator>
struct is_vector<std::vector<T, Allocator> > : std::true_type {};
}

#endif // #ifndef CXX_KINDS_HH
