#error "INCOMPLETE"

#ifndef CXX_BIND_HH
#define CXX_BIND_HH

#ifndef __has_feature
#define __has_feature(x) 0
#endif

namespace rpc {

// Movable-only bind, taken from libc++ 3.4

template <class _Arg, class _Result>
class pointer_to_unary_function : public unary_function<_Arg, _Result> {
  _Result (*__f_)(_Arg);

public:
  explicit pointer_to_unary_function(_Result (*__f)(_Arg)) : __f_(__f) {}
  _Result operator()(_Arg __x) const { return __f_(__x); }
};

template <class _Arg, class _Result>
inline pointer_to_unary_function<_Arg, _Result> ptr_fun(_Result (*__f)(_Arg)) {
  return pointer_to_unary_function<_Arg, _Result>(__f);
}

template <class _Arg1, class _Arg2, class _Result>
class pointer_to_binary_function
    : public binary_function<_Arg1, _Arg2, _Result> {
  _Result (*__f_)(_Arg1, _Arg2);

public:
  explicit pointer_to_binary_function(_Result (*__f)(_Arg1, _Arg2))
      : __f_(__f) {}
  _Result operator()(_Arg1 __x, _Arg2 __y) const { return __f_(__x, __y); }
};

template <class _Arg1, class _Arg2, class _Result>
inline pointer_to_binary_function<_Arg1, _Arg2, _Result>
ptr_fun(_Result (*__f)(_Arg1, _Arg2)) {
  return pointer_to_binary_function<_Arg1, _Arg2, _Result>(__f);
}

template <class _Sp, class _Tp>
class mem_fun_t : public unary_function<_Tp *, _Sp> {
  _Sp (_Tp::*__p_)();

public:
  explicit mem_fun_t(_Sp (_Tp::*__p)()) : __p_(__p) {}
  _Sp operator()(_Tp *__p) const { return (__p->*__p_)(); }
};

template <class _Sp, class _Tp, class _Ap>
class mem_fun1_t : public binary_function<_Tp *, _Ap, _Sp> {
  _Sp (_Tp::*__p_)(_Ap);

public:
  explicit mem_fun1_t(_Sp (_Tp::*__p)(_Ap)) : __p_(__p) {}
  _Sp operator()(_Tp *__p, _Ap __x) const { return (__p->*__p_)(__x); }
};

template <class _Sp, class _Tp>
inline mem_fun_t<_Sp, _Tp> mem_fun(_Sp (_Tp::*__f)()) {
  return mem_fun_t<_Sp, _Tp>(__f);
}

template <class _Sp, class _Tp, class _Ap>
inline mem_fun1_t<_Sp, _Tp, _Ap> mem_fun(_Sp (_Tp::*__f)(_Ap)) {
  return mem_fun1_t<_Sp, _Tp, _Ap>(__f);
}

template <class _Sp, class _Tp>
class mem_fun_ref_t : public unary_function<_Tp, _Sp> {
  _Sp (_Tp::*__p_)();

public:
  explicit mem_fun_ref_t(_Sp (_Tp::*__p)()) : __p_(__p) {}
  _Sp operator()(_Tp &__p) const { return (__p.*__p_)(); }
};

template <class _Sp, class _Tp, class _Ap>
class mem_fun1_ref_t : public binary_function<_Tp, _Ap, _Sp> {
  _Sp (_Tp::*__p_)(_Ap);

public:
  explicit mem_fun1_ref_t(_Sp (_Tp::*__p)(_Ap)) : __p_(__p) {}
  _Sp operator()(_Tp &__p, _Ap __x) const { return (__p.*__p_)(__x); }
};

template <class _Sp, class _Tp>
inline mem_fun_ref_t<_Sp, _Tp> mem_fun_ref(_Sp (_Tp::*__f)()) {
  return mem_fun_ref_t<_Sp, _Tp>(__f);
}

template <class _Sp, class _Tp, class _Ap>
inline mem_fun1_ref_t<_Sp, _Tp, _Ap> mem_fun_ref(_Sp (_Tp::*__f)(_Ap)) {
  return mem_fun1_ref_t<_Sp, _Tp, _Ap>(__f);
}

template <class _Sp, class _Tp>
class const_mem_fun_t : public unary_function<const _Tp *, _Sp> {
  _Sp (_Tp::*__p_)() const;

public:
  explicit const_mem_fun_t(_Sp (_Tp::*__p)() const) : __p_(__p) {}
  _Sp operator()(const _Tp *__p) const { return (__p->*__p_)(); }
};

template <class _Sp, class _Tp, class _Ap>
class const_mem_fun1_t : public binary_function<const _Tp *, _Ap, _Sp> {
  _Sp (_Tp::*__p_)(_Ap) const;

public:
  explicit const_mem_fun1_t(_Sp (_Tp::*__p)(_Ap) const) : __p_(__p) {}
  _Sp operator()(const _Tp *__p, _Ap __x) const { return (__p->*__p_)(__x); }
};

template <class _Sp, class _Tp>
inline const_mem_fun_t<_Sp, _Tp> mem_fun(_Sp (_Tp::*__f)() const) {
  return const_mem_fun_t<_Sp, _Tp>(__f);
}

template <class _Sp, class _Tp, class _Ap>
inline const_mem_fun1_t<_Sp, _Tp, _Ap> mem_fun(_Sp (_Tp::*__f)(_Ap) const) {
  return const_mem_fun1_t<_Sp, _Tp, _Ap>(__f);
}

template <class _Sp, class _Tp>
class const_mem_fun_ref_t : public unary_function<_Tp, _Sp> {
  _Sp (_Tp::*__p_)() const;

public:
  explicit const_mem_fun_ref_t(_Sp (_Tp::*__p)() const) : __p_(__p) {}
  _Sp operator()(const _Tp &__p) const { return (__p.*__p_)(); }
};

template <class _Sp, class _Tp, class _Ap>
class const_mem_fun1_ref_t : public binary_function<_Tp, _Ap, _Sp> {
  _Sp (_Tp::*__p_)(_Ap) const;

public:
  explicit const_mem_fun1_ref_t(_Sp (_Tp::*__p)(_Ap) const) : __p_(__p) {}
  _Sp operator()(const _Tp &__p, _Ap __x) const { return (__p.*__p_)(__x); }
};

template <class _Sp, class _Tp>
inline const_mem_fun_ref_t<_Sp, _Tp> mem_fun_ref(_Sp (_Tp::*__f)() const) {
  return const_mem_fun_ref_t<_Sp, _Tp>(__f);
}

template <class _Sp, class _Tp, class _Ap>
inline const_mem_fun1_ref_t<_Sp, _Tp, _Ap> mem_fun_ref(_Sp (_Tp::*__f)(_Ap)
                                                       const) {
  return const_mem_fun1_ref_t<_Sp, _Tp, _Ap>(__f);
}

template <class _Tp> class __mem_fn : public __weak_result_type<_Tp> {
public:
  // types
  typedef _Tp type;

private:
  type __f_;

public:
  __mem_fn(type __f) : __f_(__f) {}

  // invoke
  template <class... _ArgTypes>
  typename __invoke_return<type, _ArgTypes...>::type
  operator()(_ArgTypes &&... __args) {
    return invoke(__f_, std::forward<_ArgTypes>(__args)...);
  }
};

template <class _Rp, class _Tp>
inline __mem_fn<_Rp _Tp::*> mem_fn(_Rp _Tp::*__pm) {
  return __mem_fn<_Rp _Tp::*>(__pm);
}

// bad_function_call

class _LIBCPP_EXCEPTION_ABI bad_function_call : public exception {};

template <class _Fp> class function; // undefined

namespace __function {

template <class _Rp, class... _ArgTypes>
struct __maybe_derive_from_unary_function {};

template <class _Rp, class _A1>
struct __maybe_derive_from_unary_function<_Rp(_A1)>
    : public unary_function<_A1, _Rp> {};

template <class _Rp, class... _ArgTypes>
struct __maybe_derive_from_binary_function {};

template <class _Rp, class _A1, class _A2>
struct __maybe_derive_from_binary_function<_Rp(_A1, _A2)>
    : public binary_function<_A1, _A2, _Rp> {};

template <class _Fp> class __base;

template <class _Rp, class... _ArgTypes> class __base<_Rp(_ArgTypes...)> {
  __base(const __base &);
  __base &operator=(const __base &);

public:
  __base() {}
  virtual ~__base() {}
  virtual __base *__clone() const = 0;
  virtual void __clone(__base *) const = 0;
  virtual void destroy() _NOEXCEPT = 0;
  virtual void destroy_deallocate() _NOEXCEPT = 0;
  virtual _Rp operator()(_ArgTypes &&...) = 0;
  virtual const void *target(const type_info &) const _NOEXCEPT = 0;
  virtual const std::type_info &target_type() const _NOEXCEPT = 0;
};

template <class _FD, class _Alloc, class _FB> class __func;

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
class __func<_Fp, _Alloc, _Rp(_ArgTypes...)>
    : public __base<_Rp(_ArgTypes...)> {
  __compressed_pair<_Fp, _Alloc> __f_;

public:
  explicit __func(_Fp &&__f)
      : __f_(piecewise_construct, std::forward_as_tuple(std::move(__f)),
             std::forward_as_tuple()) {}

  explicit __func(const _Fp &__f, const _Alloc &__a)
      : __f_(piecewise_construct, std::forward_as_tuple(__f),
             std::forward_as_tuple(__a)) {}

  explicit __func(const _Fp &__f, _Alloc &&__a)
      : __f_(piecewise_construct, std::forward_as_tuple(__f),
             std::forward_as_tuple(std::move(__a))) {}

  explicit __func(_Fp &&__f, _Alloc &&__a)
      : __f_(piecewise_construct, std::forward_as_tuple(std::move(__f)),
             std::forward_as_tuple(std::move(__a))) {}
  virtual __base<_Rp(_ArgTypes...)> *__clone() const;
  virtual void __clone(__base<_Rp(_ArgTypes...)> *) const;
  virtual void destroy() _NOEXCEPT;
  virtual void destroy_deallocate() _NOEXCEPT;
  virtual _Rp operator()(_ArgTypes &&... __arg);
  virtual const void *target(const type_info &) const _NOEXCEPT;
  virtual const std::type_info &target_type() const _NOEXCEPT;
};

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
__base<_Rp(_ArgTypes...)> *
__func<_Fp, _Alloc, _Rp(_ArgTypes...)>::__clone() const {
  typedef typename _Alloc::template reunique_bind<__func>::other _Ap;
  _Ap __a(__f_.second());
  typedef __allocator_destructor<_Ap> _Dp;
  unique_ptr<__func, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
  ::new (__hold.get()) __func(__f_.first(), _Alloc(__a));
  return __hold.release();
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void __func<_Fp, _Alloc, _Rp(_ArgTypes...)>::__clone(
    __base<_Rp(_ArgTypes...)> *__p) const {
  ::new (__p) __func(__f_.first(), __f_.second());
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void __func<_Fp, _Alloc, _Rp(_ArgTypes...)>::destroy() _NOEXCEPT {
  __f_.~__compressed_pair<_Fp, _Alloc>();
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void __func<_Fp, _Alloc, _Rp(_ArgTypes...)>::destroy_deallocate() _NOEXCEPT {
  typedef typename _Alloc::template reunique_bind<__func>::other _Ap;
  _Ap __a(__f_.second());
  __f_.~__compressed_pair<_Fp, _Alloc>();
  __a.deallocate(this, 1);
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
_Rp __func<_Fp, _Alloc, _Rp(_ArgTypes...)>::operator()(_ArgTypes &&... __arg) {
  return invoke(__f_.first(), std::forward<_ArgTypes>(__arg)...);
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
const void *__func<_Fp, _Alloc, _Rp(_ArgTypes...)>::target(
    const type_info &__ti) const _NOEXCEPT {
  if (__ti == typeid(_Fp))
    return &__f_.first();
  return (const void *)0;
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
const std::type_info &
__func<_Fp, _Alloc, _Rp(_ArgTypes...)>::target_type() const _NOEXCEPT {
  return typeid(_Fp);
}

} // __function

template <class _Rp, class... _ArgTypes>
class function<_Rp(_ArgTypes...)>
    : public __function::__maybe_derive_from_unary_function<_Rp(_ArgTypes...)>,
      public __function::__maybe_derive_from_binary_function<
          _Rp(_ArgTypes...)> {
  typedef __function::__base<_Rp(_ArgTypes...)> __base;
  typename aligned_storage<3 * sizeof(void *)>::type __buf_;
  __base *__f_;

  template <class _Fp> static bool __not_null(const _Fp &) { return true; }
  template <class _R2, class... _Ap>
  static bool __not_null(_R2 (*__p)(_Ap...)) {
    return __p;
  }
  template <class _R2, class _Cp, class... _Ap>
  static bool __not_null(_R2 (_Cp::*__p)(_Ap...)) {
    return __p;
  }
  template <class _R2, class _Cp, class... _Ap>
  static bool __not_null(_R2 (_Cp::*__p)(_Ap...) const) {
    return __p;
  }
  template <class _R2, class _Cp, class... _Ap>
  static bool __not_null(_R2 (_Cp::*__p)(_Ap...) volatile) {
    return __p;
  }
  template <class _R2, class _Cp, class... _Ap>
  static bool __not_null(_R2 (_Cp::*__p)(_Ap...) const volatile) {
    return __p;
  }
  template <class _R2, class... _Ap>
  static bool __not_null(const function<_Rp(_Ap...)> &__p) {
    return __p;
  }

  template <class _Fp, bool = !is_same<_Fp, function>::value &&
                              invokable<_Fp &, _ArgTypes...>::value>
  struct __callable;
  template <class _Fp> struct __callable<_Fp, true> {
    static const bool value = is_convertible<
        typename invoke_of<_Fp &, _ArgTypes...>::type, _Rp>::value;
  };
  template <class _Fp> struct __callable<_Fp, false> {
    static const bool value = false;
  };

public:
  typedef _Rp result_type;

  // construct/copy/destroy:

  function() _NOEXCEPT : __f_(0) {}

  function(nullptr_t) _NOEXCEPT : __f_(0) {}
  function(const function &);
  function(function &&) _NOEXCEPT;
  template <class _Fp>
  function(_Fp, typename enable_if<__callable<_Fp>::value &&
                                   !is_same<_Fp, function>::value>::type * = 0);

  template <class _Alloc>
  function(allocator_arg_t, const _Alloc &) _NOEXCEPT : __f_(0) {}
  template <class _Alloc>
  function(allocator_arg_t, const _Alloc &, nullptr_t) _NOEXCEPT : __f_(0) {}
  template <class _Alloc>
  function(allocator_arg_t, const _Alloc &, const function &);
  template <class _Alloc>
  function(allocator_arg_t, const _Alloc &, function &&);
  template <class _Fp, class _Alloc>
  function(allocator_arg_t, const _Alloc &__a, _Fp __f,
           typename enable_if<__callable<_Fp>::value>::type * = 0);

  function &operator=(const function &);
  function &operator=(function &&) _NOEXCEPT;
  function &operator=(nullptr_t) _NOEXCEPT;
  template <class _Fp>
  typename enable_if<
      __callable<typename decay<_Fp>::type>::value &&
          !is_same<typename remove_reference<_Fp>::type, function>::value,
      function &>::type
  operator=(_Fp &&);

  ~function();

  // function modifiers:
  void swap(function &) _NOEXCEPT;
  template <class _Fp, class _Alloc> void assign(_Fp &&__f, const _Alloc &__a) {
    function(allocator_arg, __a, std::forward<_Fp>(__f)).swap(*this);
  }

  // function capacity:

  _LIBCPP_EXPLICIT operator bool() const _NOEXCEPT { return __f_; }

  // deleted overloads close possible hole in the type system
  template <class _R2, class... _ArgTypes2>
  bool operator==(const function<_R2(_ArgTypes2...)> &) const = delete;
  template <class _R2, class... _ArgTypes2>
  bool operator!=(const function<_R2(_ArgTypes2...)> &) const = delete;

public:
  // function invocation:
  _Rp operator()(_ArgTypes...) const;

  // function target access:
  const std::type_info &target_type() const _NOEXCEPT;
  template <typename _Tp> _Tp *target() _NOEXCEPT;
  template <typename _Tp> const _Tp *target() const _NOEXCEPT;
};

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)>::function(const function &__f) {
  if (__f.__f_ == 0)
    __f_ = 0;
  else if (__f.__f_ == (const __base *)&__f.__buf_) {
    __f_ = (__base *)&__buf_;
    __f.__f_->__clone(__f_);
  } else
    __f_ = __f.__f_->__clone();
}

template <class _Rp, class... _ArgTypes>
template <class _Alloc>
function<_Rp(_ArgTypes...)>::function(allocator_arg_t, const _Alloc &,
                                      const function &__f) {
  if (__f.__f_ == 0)
    __f_ = 0;
  else if (__f.__f_ == (const __base *)&__f.__buf_) {
    __f_ = (__base *)&__buf_;
    __f.__f_->__clone(__f_);
  } else
    __f_ = __f.__f_->__clone();
}

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)>::function(function &&__f) _NOEXCEPT {
  if (__f.__f_ == 0)
    __f_ = 0;
  else if (__f.__f_ == (__base *)&__f.__buf_) {
    __f_ = (__base *)&__buf_;
    __f.__f_->__clone(__f_);
  } else {
    __f_ = __f.__f_;
    __f.__f_ = 0;
  }
}

template <class _Rp, class... _ArgTypes>
template <class _Alloc>
function<_Rp(_ArgTypes...)>::function(allocator_arg_t, const _Alloc &,
                                      function &&__f) {
  if (__f.__f_ == 0)
    __f_ = 0;
  else if (__f.__f_ == (__base *)&__f.__buf_) {
    __f_ = (__base *)&__buf_;
    __f.__f_->__clone(__f_);
  } else {
    __f_ = __f.__f_;
    __f.__f_ = 0;
  }
}

template <class _Rp, class... _ArgTypes>
template <class _Fp>
function<_Rp(_ArgTypes...)>::function(
    _Fp __f, typename enable_if<__callable<_Fp>::value &&
                                !is_same<_Fp, function>::value>::type *)
    : __f_(0) {
  if (__not_null(__f)) {
    typedef __function::__func<_Fp, allocator<_Fp>, _Rp(_ArgTypes...)> _FF;
    if (sizeof(_FF) <= sizeof(__buf_) &&
        is_nothrow_copy_constructible<_Fp>::value) {
      __f_ = (__base *)&__buf_;
      ::new (__f_) _FF(std::move(__f));
    } else {
      typedef allocator<_FF> _Ap;
      _Ap __a;
      typedef __allocator_destructor<_Ap> _Dp;
      unique_ptr<__base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
      ::new (__hold.get()) _FF(std::move(__f), allocator<_Fp>(__a));
      __f_ = __hold.release();
    }
  }
}

template <class _Rp, class... _ArgTypes>
template <class _Fp, class _Alloc>
function<_Rp(_ArgTypes...)>::function(
    allocator_arg_t, const _Alloc &__a0, _Fp __f,
    typename enable_if<__callable<_Fp>::value>::type *)
    : __f_(0) {
  typedef allocator_traits<_Alloc> __alloc_traits;
  if (__not_null(__f)) {
    typedef __function::__func<_Fp, _Alloc, _Rp(_ArgTypes...)> _FF;
    if (sizeof(_FF) <= sizeof(__buf_) &&
        is_nothrow_copy_constructible<_Fp>::value) {
      __f_ = (__base *)&__buf_;
      ::new (__f_) _FF(std::move(__f));
    } else {
      typedef typename __alloc_traits::template reunique_bind_alloc<_FF> _Ap;
      _Ap __a(__a0);
      typedef __allocator_destructor<_Ap> _Dp;
      unique_ptr<__base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
      ::new (__hold.get()) _FF(std::move(__f), _Alloc(__a));
      __f_ = __hold.release();
    }
  }
}

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)> &function<_Rp(_ArgTypes...)>::
operator=(const function &__f) {
  function(__f).swap(*this);
  return *this;
}

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)> &function<_Rp(_ArgTypes...)>::
operator=(function &&__f) _NOEXCEPT {
  if (__f_ == (__base *)&__buf_)
    __f_->destroy();
  else if (__f_)
    __f_->destroy_deallocate();
  __f_ = 0;
  if (__f.__f_ == 0)
    __f_ = 0;
  else if (__f.__f_ == (__base *)&__f.__buf_) {
    __f_ = (__base *)&__buf_;
    __f.__f_->__clone(__f_);
  } else {
    __f_ = __f.__f_;
    __f.__f_ = 0;
  }
  return *this;
}

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)> &function<_Rp(_ArgTypes...)>::
operator=(nullptr_t) _NOEXCEPT {
  if (__f_ == (__base *)&__buf_)
    __f_->destroy();
  else if (__f_)
    __f_->destroy_deallocate();
  __f_ = 0;
  return *this;
}

template <class _Rp, class... _ArgTypes>
template <class _Fp>
typename enable_if<function<_Rp(_ArgTypes...)>::template __callable<
                       typename decay<_Fp>::type>::value &&
                       !is_same<typename remove_reference<_Fp>::type,
                                function<_Rp(_ArgTypes...)> >::value,
                   function<_Rp(_ArgTypes...)> &>::type
    function<_Rp(_ArgTypes...)>::
    operator=(_Fp &&__f) {
  function(std::forward<_Fp>(__f)).swap(*this);
  return *this;
}

template <class _Rp, class... _ArgTypes>
function<_Rp(_ArgTypes...)>::~function() {
  if (__f_ == (__base *)&__buf_)
    __f_->destroy();
  else if (__f_)
    __f_->destroy_deallocate();
}

template <class _Rp, class... _ArgTypes>
void function<_Rp(_ArgTypes...)>::swap(function &__f) _NOEXCEPT {
  if (__f_ == (__base *)&__buf_ && __f.__f_ == (__base *)&__f.__buf_) {
    typename aligned_storage<sizeof(__buf_)>::type __tempbuf;
    __base *__t = (__base *)&__tempbuf;
    __f_->__clone(__t);
    __f_->destroy();
    __f_ = 0;
    __f.__f_->__clone((__base *)&__buf_);
    __f.__f_->destroy();
    __f.__f_ = 0;
    __f_ = (__base *)&__buf_;
    __t->__clone((__base *)&__f.__buf_);
    __t->destroy();
    __f.__f_ = (__base *)&__f.__buf_;
  } else if (__f_ == (__base *)&__buf_) {
    __f_->__clone((__base *)&__f.__buf_);
    __f_->destroy();
    __f_ = __f.__f_;
    __f.__f_ = (__base *)&__f.__buf_;
  } else if (__f.__f_ == (__base *)&__f.__buf_) {
    __f.__f_->__clone((__base *)&__buf_);
    __f.__f_->destroy();
    __f.__f_ = __f_;
    __f_ = (__base *)&__buf_;
  } else
    std::swap(__f_, __f.__f_);
}

template <class _Rp, class... _ArgTypes>
_Rp function<_Rp(_ArgTypes...)>::operator()(_ArgTypes... __arg) const {
  if (__f_ == 0)
    throw bad_function_call();
  return (*__f_)(std::forward<_ArgTypes>(__arg)...);
}

template <class _Rp, class... _ArgTypes>
const std::type_info &
function<_Rp(_ArgTypes...)>::target_type() const _NOEXCEPT {
  if (__f_ == 0)
    return typeid(void);
  return __f_->target_type();
}

template <class _Rp, class... _ArgTypes>
template <typename _Tp>
_Tp *function<_Rp(_ArgTypes...)>::target() _NOEXCEPT {
  if (__f_ == 0)
    return (_Tp *)0;
  return (_Tp *)__f_->target(typeid(_Tp));
}

template <class _Rp, class... _ArgTypes>
template <typename _Tp>
const _Tp *function<_Rp(_ArgTypes...)>::target() const _NOEXCEPT {
  if (__f_ == 0)
    return (const _Tp *)0;
  return (const _Tp *)__f_->target(typeid(_Tp));
}

template <class _Rp, class... _ArgTypes>
inline bool operator==(const function<_Rp(_ArgTypes...)> &__f,
                       nullptr_t) _NOEXCEPT {
  return !__f;
}

template <class _Rp, class... _ArgTypes>
inline bool operator==(nullptr_t,
                       const function<_Rp(_ArgTypes...)> &__f) _NOEXCEPT {
  return !__f;
}

template <class _Rp, class... _ArgTypes>
inline bool operator!=(const function<_Rp(_ArgTypes...)> &__f,
                       nullptr_t) _NOEXCEPT {
  return (bool)__f;
}

template <class _Rp, class... _ArgTypes>
inline bool operator!=(nullptr_t,
                       const function<_Rp(_ArgTypes...)> &__f) _NOEXCEPT {
  return (bool)__f;
}

template <class _Rp, class... _ArgTypes>
inline void swap(function<_Rp(_ArgTypes...)> &__x,
                 function<_Rp(_ArgTypes...)> &__y) _NOEXCEPT {
  return __x.swap(__y);
}

template <class _Tp> struct __is_unique_bind_expression : public false_type {};
template <class _Tp>
struct is_unique_bind_expression
    : public __is_unique_bind_expression<typename remove_cv<_Tp>::type> {};

template <class _Tp>
struct __is_placeholder : public integral_constant<int, 0> {};
template <class _Tp>
struct is_placeholder : public __is_placeholder<typename remove_cv<_Tp>::type> {
};

namespace placeholders {

template <int _Np> struct __ph {};

_LIBCPP_FUNC_VIS extern __ph<1> _1;
_LIBCPP_FUNC_VIS extern __ph<2> _2;
_LIBCPP_FUNC_VIS extern __ph<3> _3;
_LIBCPP_FUNC_VIS extern __ph<4> _4;
_LIBCPP_FUNC_VIS extern __ph<5> _5;
_LIBCPP_FUNC_VIS extern __ph<6> _6;
_LIBCPP_FUNC_VIS extern __ph<7> _7;
_LIBCPP_FUNC_VIS extern __ph<8> _8;
_LIBCPP_FUNC_VIS extern __ph<9> _9;
_LIBCPP_FUNC_VIS extern __ph<10> _10;

} // placeholders

template <int _Np>
struct __is_placeholder<placeholders::__ph<_Np> >
    : public integral_constant<int, _Np> {};

template <class _Tp, class _Uj>
inline _Tp &__mu(reference_wrapper<_Tp> __t, _Uj &) {
  return __t.get();
}

template <class _Ti, class... _Uj, size_t... _Indx>
inline typename invoke_of<_Ti &, _Uj...>::type
__mu_expand(_Ti &__ti, tuple<_Uj...> &__uj, __tuple_indices<_Indx...>) {
  return __ti(std::forward<_Uj>(get<_Indx>(__uj))...);
}

template <class _Ti, class... _Uj>
inline typename enable_if<is_unique_bind_expression<_Ti>::value,
                          typename invoke_of<_Ti &, _Uj...>::type>::type
__mu(_Ti &__ti, tuple<_Uj...> &__uj) {
  typedef typename __make_tuple_indices<sizeof...(_Uj)>::type __indices;
  return __mu_expand(__ti, __uj, __indices());
}

template <bool IsPh, class _Ti, class _Uj> struct __mu_return2 {};

template <class _Ti, class _Uj> struct __mu_return2<true, _Ti, _Uj> {
  typedef typename tuple_element<is_placeholder<_Ti>::value - 1, _Uj>::type
      type;
};

template <class _Ti, class _Uj>
inline typename enable_if<
    0 < is_placeholder<_Ti>::value,
    typename __mu_return2<0 < is_placeholder<_Ti>::value, _Ti, _Uj>::type>::type
__mu(_Ti &, _Uj &__uj) {
  const size_t _Indx = is_placeholder<_Ti>::value - 1;
  return std::forward<typename tuple_element<_Indx, _Uj>::type>(
      get<_Indx>(__uj));
}

template <class _Ti, class _Uj>
inline typename enable_if<!is_unique_bind_expression<_Ti>::value &&
                              is_placeholder<_Ti>::value == 0 &&
                              !__is_reference_wrapper<_Ti>::value,
                          _Ti &>::type
__mu(_Ti &__ti, _Uj &) {
  return __ti;
}

template <class _Ti, bool IsReferenceWrapper, bool IsUnique_BindEx, bool IsPh,
          class _TupleUj>
struct ____mu_return;

template <bool _Invokable, class _Ti, class... _Uj>
struct ____mu_return_invokable // false
    {
  typedef __nat type;
};

template <class _Ti, class... _Uj>
struct ____mu_return_invokable<true, _Ti, _Uj...> {
  typedef typename invoke_of<_Ti &, _Uj...>::type type;
};

template <class _Ti, class... _Uj>
struct ____mu_return<_Ti, false, true, false, tuple<_Uj...> >
    : public ____mu_return_invokable<invokable<_Ti &, _Uj...>::value, _Ti,
                                     _Uj...> {};

template <class _Ti, class _TupleUj>
struct ____mu_return<_Ti, false, false, true, _TupleUj> {
  typedef typename tuple_element<is_placeholder<_Ti>::value - 1,
                                 _TupleUj>::type &&type;
};

template <class _Ti, class _TupleUj>
struct ____mu_return<_Ti, true, false, false, _TupleUj> {
  typedef typename _Ti::type &type;
};

template <class _Ti, class _TupleUj>
struct ____mu_return<_Ti, false, false, false, _TupleUj> {
  typedef _Ti &type;
};

template <class _Ti, class _TupleUj>
struct __mu_return
    : public ____mu_return<_Ti, __is_reference_wrapper<_Ti>::value,
                           is_unique_bind_expression<_Ti>::value,
                           0 < is_placeholder<_Ti>::value &&
                               is_placeholder<_Ti>::value <=
                                   tuple_size<_TupleUj>::value,
                           _TupleUj> {};

template <class _Fp, class _BoundArgs, class _TupleUj>
struct _is_valid_unique_bind_return {
  static const bool value = false;
};

template <class _Fp, class... _BoundArgs, class _TupleUj>
struct _is_valid_unique_bind_return<_Fp, tuple<_BoundArgs...>, _TupleUj> {
  static const bool value = invokable<
      _Fp, typename __mu_return<_BoundArgs, _TupleUj>::type...>::value;
};

template <class _Fp, class... _BoundArgs, class _TupleUj>
struct _is_valid_unique_bind_return<_Fp, const tuple<_BoundArgs...>, _TupleUj> {
  static const bool value = invokable<
      _Fp, typename __mu_return<const _BoundArgs, _TupleUj>::type...>::value;
};

template <class _Fp, class _BoundArgs, class _TupleUj,
          bool = _is_valid_unique_bind_return<_Fp, _BoundArgs, _TupleUj>::value>
struct __unique_bind_return;

template <class _Fp, class... _BoundArgs, class _TupleUj>
struct __unique_bind_return<_Fp, tuple<_BoundArgs...>, _TupleUj, true> {
  typedef typename invoke_of<
      _Fp &, typename __mu_return<_BoundArgs, _TupleUj>::type...>::type type;
};

template <class _Fp, class... _BoundArgs, class _TupleUj>
struct __unique_bind_return<_Fp, const tuple<_BoundArgs...>, _TupleUj, true> {
  typedef typename invoke_of<
      _Fp &, typename __mu_return<const _BoundArgs, _TupleUj>::type...>::type
      type;
};

template <class _Fp, class _BoundArgs, size_t... _Indx, class _Args>
inline typename __unique_bind_return<_Fp, _BoundArgs, _Args>::type
__apply_unique_functor(_Fp &&__f, _BoundArgs &&__bound_args,
                       __tuple_indices<_Indx...>, _Args &&__args) {
  return invoke(__f, __mu(get<_Indx>(__bound_args), __args)...);
}

template <class _Fp, class... _BoundArgs>
class __unique_bind : public __weak_result_type<typename decay<_Fp>::type> {
protected:
  typedef typename decay<_Fp>::type _Fd;
  typedef tuple<typename decay<_BoundArgs>::type...> _Td;

private:
  _Fd __f_;
  _Td __bound_args_;

  typedef typename __make_tuple_indices<sizeof...(_BoundArgs)>::type __indices;

public:
  template <
      class _Gp, class... _BA,
      class = typename enable_if<is_constructible<_Fd, _Gp>::value &&
                                 !is_same<typename remove_reference<_Gp>::type,
                                          __unique_bind>::value>::type>
  explicit __unique_bind(_Gp &&__f, _BA &&... __bound_args)
      : __f_(std::forward<_Gp>(__f)),
        __bound_args_(std::forward<_BA>(__bound_args)...) {}

  template <class... _Args>
  typename __unique_bind_return<_Fd, _Td, tuple<_Args &&...> >::type
  operator()(_Args &&... __args) {
    return __apply_unique_functor(__f_, __bound_args_, __indices(),
                                  tuple < _Args &&
                                      ... > (std::forward<_Args>(__args)...));
  }

  template <class... _Args>
  typename __unique_bind_return<const _Fd, const _Td, tuple<_Args &&...> >::type
  operator()(_Args &&... __args) const {
    return __apply_unique_functor(__f_, __bound_args_, __indices(),
                                  tuple < _Args &&
                                      ... > (std::forward<_Args>(__args)...));
  }
};

template <class _Fp, class... _BoundArgs>
struct __is_unique_bind_expression<__unique_bind<_Fp, _BoundArgs...> >
    : public true_type {};

template <class _Rp, class _Fp, class... _BoundArgs>
class __unique_bind_r : public __unique_bind<_Fp, _BoundArgs...> {
  typedef __unique_bind<_Fp, _BoundArgs...> base;
  typedef typename base::_Fd _Fd;
  typedef typename base::_Td _Td;

public:
  typedef _Rp result_type;

  template <
      class _Gp, class... _BA,
      class = typename enable_if<is_constructible<_Fd, _Gp>::value &&
                                 !is_same<typename remove_reference<_Gp>::type,
                                          __unique_bind_r>::value>::type>
  explicit __unique_bind_r(_Gp &&__f, _BA &&... __bound_args)
      : base(std::forward<_Gp>(__f), std::forward<_BA>(__bound_args)...) {}

  template <class... _Args>
  typename enable_if<is_convertible<typename __unique_bind_return<
                                        _Fd, _Td, tuple<_Args &&...> >::type,
                                    result_type>::value,
                     result_type>::type
  operator()(_Args &&... __args) {
    return base::operator()(std::forward<_Args>(__args)...);
  }

  template <class... _Args>
  typename enable_if<
      is_convertible<typename __unique_bind_return<const _Fd, const _Td,
                                                   tuple<_Args &&...> >::type,
                     result_type>::value,
      result_type>::type
  operator()(_Args &&... __args) const {
    return base::operator()(std::forward<_Args>(__args)...);
  }
};

template <class _Rp, class _Fp, class... _BoundArgs>
struct __is_unique_bind_expression<__unique_bind_r<_Rp, _Fp, _BoundArgs...> >
    : public true_type {};

template <class _Fp, class... _BoundArgs>
inline __unique_bind<_Fp, _BoundArgs...>
unique_bind(_Fp &&__f, _BoundArgs &&... __bound_args) {
  typedef __unique_bind<_Fp, _BoundArgs...> type;
  return type(std::forward<_Fp>(__f),
              std::forward<_BoundArgs>(__bound_args)...);
}

template <class _Rp, class _Fp, class... _BoundArgs>
inline __unique_bind_r<_Rp, _Fp, _BoundArgs...>
unique_bind(_Fp &&__f, _BoundArgs &&... __bound_args) {
  typedef __unique_bind_r<_Rp, _Fp, _BoundArgs...> type;
  return type(std::forward<_Fp>(__f),
              std::forward<_BoundArgs>(__bound_args)...);
}
}

#define CXX_BIND_HH_DONE
#else
#ifndef CXX_BIND_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // CXX_BIND_HH
