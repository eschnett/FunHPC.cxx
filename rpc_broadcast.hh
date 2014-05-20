#ifndef RPC_BROADCAST_HH
#define RPC_BROADCAST_HH

#include "rpc_call.hh"
#include "rpc_client_fwd.hh"

#include "cxx_utils.hh"

#include <cassert>
#include <iterator>
#include <type_traits>
#include <vector>

namespace rpc {
  
  using std::enable_if;
  using std::forward;
  using std::is_same;
  using std::iterator_traits;
  using std::vector;
  
  
  
  vector<int> find_all_processes();
  vector<int> find_all_threads();
  
  
  
  // Broadcast to all processes
  template<typename C, typename F, typename... As>
  auto broadcast(const C& dests, const F& func, const As&... args) ->
    typename enable_if
    <is_action<F>::value,
     vector<future<typename invoke_of<F, As...>::type> > >::type
  {
    vector<future<typename invoke_of<F, As...>::type> > fs;
    // TODO: use tree
    for (const int dest: dests) {
      fs.push_back(async(remote::async, dest, func, args...));
    }
    return fs;
  }
  
  template<typename C, typename F, typename... As>
  auto broadcast_detached(const C& dests, const F& func, const As&... args) ->
    typename enable_if<is_action<F>::value, void>::type
  {
    // TODO: use tree
    for (const int dest: dests) {
      detached(remote::detached, dest, func, args...);
    }
  }
  
  // TODO: Use container instead of b and e?
  template<typename F, typename... As>
  auto broadcast_barrier(F func, const As&... args, int b=0, int e=-1) ->
    typename enable_if<is_action<F>::value, future<void> >::type
  {
    if (e == -1) e = server->size();
    const auto sz = e - b;
    RPC_ASSERT(sz > 0);
    // TODO: execute on different processes
    // TODO: create an action for this?
    if (sz == 1) return async(remote::async, b, func, args...);
    const auto m = b + sz/2;
    const auto fs0 = broadcast_barrier(func, args..., b, m);
    const auto fs1 = broadcast_barrier(func, args..., m, e);
    return rpc::async([=](){ fs0.wait(); fs1.wait(); });
  }
  
  
  
#if 0
  // Map-reduce from futures
  //   const F& must be A -> B
  //   R must be (B, B) -> B
  //   Z must be () -> B
  //   I must be iterator over [A]
  template<typename F, typename R, typename I>
  auto map_reduce1(const F& f, const R& op, const I& b, const I& e) ->
    decltype(f(*b))
  {
    const auto sz = e - b;
    RPC_ASSERT(sz != 0);
    // TODO: execute on different processes
    if (sz == 1) return f(*b);
    const auto m = b + sz/2;
    return op(reduce1(op, b, m), reduce1(op, m, e));
  }
  
  template<typename F, typename R, typename Z, typename I>
  auto map_reduce(const F& f, const R& op, Z& zero, const I& b, const I& e) ->
    decltype(f(*b))
  {
    if (b == e) return zero();
    return reduce1(f, op, b, e);
  }
  
  
  
  // Reduce from futures
  template<typename R, typename I>
  auto reduce1(const R& op, const I& b, const I& e) -> decltype(op(*b, *b))
  {
    const auto sz = e - b;
    RPC_ASSERT(sz != 0);
    // TODO: execute on different processes
    if (sz == 1) return *b;
    const auto m = b + sz/2;
    return op(reduce1(op, b, m), reduce1(op, m, e));
  }
  
  template<typename R, typename Z, typename I>
  auto reduce(const R& op, Z& zero, const I& b, const I& e) ->
    decltype(op(*b, *b))
  // TODO: R must be FV(FV,FV)
  // TODO: Z must be FV()
  // TODO: I must be iterator over FV
  // NOTE: this currently also works with V instead of FV
  // TODO: accept global_ptr<future>?
  {
    if (b == e) return zero();
    return reduce1(op, b, e);
  }
#endif
  
  
  
  // Map-reduce from clients
  //   function:  F :: A -> B
  //   reduction: R :: (B, B) -> B
  //   zero:      Z :: B
  //   container: C :: [A]
  //   iterator:  I :: iterator over A (TODO: and into C)
  
  template<typename A, typename B, typename F, typename R, typename Z,
           typename C, typename I>
  struct map_reduce_impl {
    F f;
    R r;
    Z z;
    client<C> c;
    map_reduce_impl(const F& f, const R& r, const Z& z, const client<C>& c):
      f(f), r(r), z(z), c(c)
    {
      RPC_ASSERT(c.is_local());
    }
    
    // Assert the type relationships above
    static_assert(std::is_same<typename invoke_of<F, A>::type, B>::value,
                  "");
    static_assert(std::is_same<typename invoke_of<R, B, B>::type, B>::value,
                  "");
    static_assert(std::is_same<typename invoke_of<Z>::type, B>::value,
                  "");
    static_assert(std::is_same<typename C::value_type, A>::value,
                  "");
    static_assert(std::is_same<typename iterator_traits<I>::value_type, A>::value,
                  "");
    
    // Assert that B can be constructed from a future to B
    static_assert(std::is_convertible<future<B>, B>::value,
                  "");
    
    B map_reduce1(const I& b, const I& e) const
    {
      const auto sz = e - b;
      RPC_ASSERT(sz > 0);
      if (sz == 1) {
        return async(f, *b);
      }
      const auto m = b + sz/2;
      B res1 = map_reduce1(b, m);
      B res2 = map_reduce1(m, e);
      // B res1 = async(&map_reduce_impl::map_reduce1, this, b, m);
      // B res2 = async(&map_reduce_impl::map_reduce1, this, m, e);
      return async(r, res1, res2);
    }
    
    B map_reduce(const I& b, const I& e) const
    {
      if (b == e) return z();
      return map_reduce1(b, e);
    }
  };
  
  
  
  template<typename F, typename R, typename C, typename I>
  auto map_reduce1(const F& f, const R& r, const client<C>& c,
                   const I& b, const I& e) ->
    typename invoke_of<F, typename C::value_type>::type
  {
    typedef typename C::value_type A;
    typedef typename invoke_of<F, A>::type B;
    auto z = []()->B{std::terminate();};
    typedef typename std::decay<decltype(z)>::type Z;
    return map_reduce_impl<A, B, F, R, Z, C, I>(f, r, z, c).map_reduce1(b, e);
  }
  
  template<typename F, typename R, typename C>
  auto map_reduce1(const F& f, const R& r, const client<C>& c) ->
    typename invoke_of<F, typename C::value_type>::type
  {
    return map_reduce1(f, r, c, c->begin(), c->end());
  }
  
  template<typename F, typename R, typename Z, typename C, typename I>
  auto map_reduce(const F& f, const R& r, const Z& z, const client<C>& c,
                  const I& b, const I& e) ->
    typename std::decay<decltype(f(*b))>::type
  {
    typedef typename C::value_type A;
    typedef typename invoke_of<F, A>::type B;
    return map_reduce_impl<A, B, F, R, Z, C, I>(f, r, z, c).map_reduce(b, e);
  }
  
  template<typename F, typename R, typename Z, typename C>
  auto map_reduce(const F& f, const R& r, const Z& z, const C& c) ->
    typename std::decay<decltype(f(*c->begin()))>::type
  {
    return map_reduce(f, r, z, c, c->begin(), c->end());
  }
  
  
  
  template<typename A, typename B, typename R, typename Z,
           typename C, typename I>
  struct reduce_impl {
    R r;
    Z z;
    client<C> c;
    reduce_impl(const R& r, const Z& z, const client<C>& c):
      r(r), z(z), c(c)
    {
      RPC_ASSERT(c.is_local());
    }
    
    // Assert the type relationships above
    static_assert(std::is_same<A, B>::value,
                  "");
    static_assert(std::is_same<typename invoke_of<R, B, B>::type, B>::value,
                  "");
    static_assert(std::is_same<typename invoke_of<Z>::type, B>::value,
                  "");
    static_assert(std::is_same<typename C::value_type, A>::value,
                  "");
    static_assert(std::is_same<typename iterator_traits<I>::value_type, A>::value,
                  "");
    
    // Assert that B can be constructed from a future to B
    static_assert(std::is_convertible<future<B>, B>::value,
                  "");
    
    B reduce1(const I& b, const I& e) const
    {
      const auto sz = e - b;
      RPC_ASSERT(sz > 0);
      if (sz == 1) {
        return *b;
      }
      const auto m = b + sz/2;
      B res1 = reduce1(b, m);
      B res2 = reduce1(m, e);
      // B res1 = async(&reduce_impl::reduce1, this, b, m);
      // B res2 = async(&reduce_impl::reduce1, this, m, e);
      return async(r, res1, res2);
    }
    
    B reduce(const I& b, const I& e) const
    {
      if (b == e) return z();
      return reduce1(b, e);
    }
  };
  
  
  
  template<typename R, typename C, typename I>
  auto reduce1(const R& r, const client<C>& c,
               const I& b, const I& e) ->
    typename C::value_type
  {
    typedef typename C::value_type A;
    typedef A B;
    auto z = []()->B{std::terminate();};
    typedef typename std::decay<decltype(z)>::type Z;
    return reduce_impl<A, B, R, Z, C, I>(r, z, c).reduce1(b, e);
  }
  
  template<typename R, typename C>
  auto reduce1(const R& r, const C& c) ->
    typename C::value_type
  {
    return reduce1(r, c, c->begin(), c->end());
  }
  
  template<typename R, typename Z, typename C, typename I>
  auto reduce(const R& r, const Z& z, const client<C>& c,
              const I& b, const I& e) ->
    typename C::value_type
  {
    typedef typename C::value_type A;
    typedef A B;
    return reduce_impl<A, B, R, Z, C, I>(r, z, c).reduce(b, e);
  }
  
  template<typename R, typename Z, typename C>
  auto reduce(const R& r, const Z& z, const C& c) ->
    typename C::value_type
  {
    return reduce(r, z, c, c->begin(), c->end());
  }
}

#define RPC_BROADCAST_HH_DONE
#else
#  ifndef RPC_BROADCAST_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // #ifndef RPC_BROADCAST_HH
