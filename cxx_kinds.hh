#ifndef CXX_KINDS_HH
#define CXX_KINDS_HH

#include <array>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>

namespace cxx {

template <typename> struct kinds;

// array
template <typename T, std::size_t N> struct kinds<std::array<T, N> > {
  typedef T type;
  template <typename U> using constructor = std::array<U, N>;
};

// function
template <typename T, typename A> struct kinds<std::function<T(A)> > {
  typedef T type;
  template <typename U> using constructor = std::function<U(A)>;
};

// list
template <typename T, typename Allocator>
struct kinds<std::list<T, Allocator> > {
  typedef T type;
  template <typename U> using constructor = std::list<U>;
};

// set
template <typename T, typename Compare, typename Allocator>
struct kinds<std::set<T, Compare, Allocator> > {
  typedef T type;
  template <typename U> using constructor = std::set<U>;
};

// shared_ptr
template <typename T> struct kinds<std::shared_ptr<T> > {
  typedef T type;
  template <typename U> using constructor = std::shared_ptr<U>;
};

// vector
template <typename T, typename Allocator>
struct kinds<std::vector<T, Allocator> > {
  typedef T type;
  template <typename U> using constructor = std::vector<U>;
};
}

#endif // #ifndef CXX_KINDS_HH
