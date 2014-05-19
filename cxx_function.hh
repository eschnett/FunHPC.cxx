#error "INCOMPLETE"

#ifndef CXX_FUNCTION_HH
#define CXX_FUNCTION_HH

#ifndef __has_feature
#  define __has_feature(x) 0
#endif

namespace rpc {
  
  
  
  // movable-only function
  
  namespace detail {
    
    template<typename>
    struct unique_function_impl_base;
    template<typename R, typename... Args>
    struct unique_function_impl_base<R(Args...)> {
      typedef R result_type;
      virtual ~unique_function_impl_base() {}
      virtual R operator()(Args&&... args) = 0;
    };
    
    template<typename, typename>
    struct unique_function_impl;
    template<typename F, typename R, typename... Args>
    struct unique_function_impl<F, R(Args...)>:
      public unique_function_impl_base<R(Args...)>
    {
      F f;
      unique_function_impl(F&& f): f(std::move(f)) {}
      virtual ~unique_function_impl() {}
      virtual R operator()(Args&&... args)
      {
        return rpc::invoke(std::move(f), std::forward<Args>(args)...);
      }
    };
    
  }
  
  
  
  template<typename>
  class unique_function;
  
  template<typename R, typename... Args>
  class unique_function<R(Args...)>
  {
    typedef R result_type;
    
  private:
    std::unique_ptr<detail::unique_function_impl_base<R(Args...)> > impl;
  public:
    unique_function(): impl(nullptr) {}
    unique_function(std::nullptr_t): unique_function() {}
    unique_function(unique_function&& other): impl(std::move(other.impl)) {}
    template<typename F>
    unique_function(F&& f):
      impl(rpc::make_unique<detail::unique_function_impl<F, R(Args...)> >
           (std::forward<F>(f)))
    {
    }
    unique_function& operator=(unique_function&& other)
    {
      impl = other.impl;
      return *this;
    }
    void swap(unique_function& other) { std::swap(impl, other.impl); }
    
    operator bool() const { return impl; }
    R operator()(Args&&... args)
    {
      return rpc::invoke(std::move(impl), std::forward<Args>(args)...);
    }
    std::function<R(Args...)> share()
    {
      std::function<R(Args...)> f;
      if (bool(*this)) f = impl->f;
      return f;
    }
  };
  
  template<typename R, typename... Args>
  void swap(unique_function<R(Args...)>& lhs, unique_function<R(Args...)>& rhs)
  {
    lhs.swap(rhs);
  }
  
}

#define CXX_FUNCTION_HH_DONE
#else
#  ifndef CXX_FUNCTION_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // CXX_FUNCTION_HH
