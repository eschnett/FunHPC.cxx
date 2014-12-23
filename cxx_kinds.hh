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

template <typename> struct is_async : std::false_type {};

// array
template <typename T, size_t N> struct kinds<std::array<T, N> > {
  typedef T value_type;
  template <typename U> using constructor = std::array<U, N>;
};
template <typename T> struct is_array : std::false_type {};
template <typename T, size_t N>
struct is_array<std::array<T, N> > : std::true_type {};

// function
template <typename T, typename A> struct kinds<std::function<T(A)> > {
  typedef T value_type;
  template <typename U> using constructor = std::function<U(A)>;
};
template <typename T> struct is_function : std::false_type {};
template <typename T, typename A>
struct is_function<std::function<T(A)> > : std::true_type {};

// list
template <typename T> struct kinds<std::list<T> > {
  typedef T value_type;
  template <typename U> using constructor = std::list<U>;
};
template <typename T> struct is_list : std::false_type {};
template <typename T> struct is_list<std::list<T> > : std::true_type {};

// set
template <typename T, typename Compare, typename Allocator>
struct kinds<std::set<T, Compare, Allocator> > {
  typedef T value_type;
  template <typename U>
  using constructor = std::set<
      U /*TODO std::less<U>, typename Allocator::template rebind<U>::other*/>;
};
template <typename T> struct is_set : std::false_type {};
template <typename T, typename Compare, typename Allocator>
struct is_set<std::set<T, Compare, Allocator> > : std::true_type {};

// shared_ptr
template <typename T> struct kinds<std::shared_ptr<T> > {
  typedef T value_type;
  template <typename U> using constructor = std::shared_ptr<U>;
};
template <typename T> struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T> > : std::true_type {};

// vector
template <typename T> struct kinds<std::vector<T> > {
  typedef T value_type;
  template <typename U> using constructor = std::vector<U>;
};
template <typename T> struct is_vector : std::false_type {};
template <typename T> struct is_vector<std::vector<T> > : std::true_type {};
}

#define CXX_KINDS_HH_DONE
#else
#ifndef CXX_KINDS_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifdef CXX_KINDS_HH
