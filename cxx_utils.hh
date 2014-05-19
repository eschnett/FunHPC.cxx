#ifndef CXX_UTILS_HH
#define CXX_UTILS_HH

#include <cassert>
#include <iostream>
#include <memory>

#ifndef __has_feature
#  define __has_feature(x) 0
#endif

#ifndef RPC_ASSERT
#  define RPC_ASSERT(x) assert(x)
#endif

namespace rpc {
  
  
  
  // decay_copy, taken from libc++ 3.4
  
  template <class _Tp>
  inline
  typename std::decay<_Tp>::type
  decay_copy(const _Tp& __t)
  {
    return std::forward<_Tp>(__t);
  }
  
  
  
  // Assert wrapper
  
  inline void rpc_assert(bool cond)
  {
    if (cond) return;
    std::cout << "ASSERTION FAILURE\n" << std::flush;
    std::cerr << "ASSERTION FAILURE\n" << std::flush;
    assert(0);
  }
  
  
  
  // // make_ptr
  // 
  // template<typename T>
  // auto make_ptr(T&& value) ->
  //   decltype(new typename std::decay<T>::type(std::forward<T>(value)))
  // {
  //   return new typename std::decay<T>::type(std::forward<T>(value));
  // }
  
  
  
  // make_unique
  
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
  
}

#define CXX_UTILS_HH_DONE
#else
#  ifndef CXX_UTILS_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // CXX_UTILS_HH
