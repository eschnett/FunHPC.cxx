#ifndef MATRIX_RPC_HH
#define MATRIX_RPC_HH

#include "matrix.hh"

#include "rpc.hh"

#include <boost/shared_ptr.hpp>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>



struct matrix_t_client;

// TODO: use managed_component_base
struct vector_t_component:
  public rpc::components::simple_component_base<vector_t_component>
{
  vector_t::ptr data;
  
  vector_t_component(std::ptrdiff_t N): data(boost::make_shared<vector_t>(N)) {}
  vector_t_component& operator=(const vector_t_component&) = delete;
  vector_t_component(const vector_t::ptr& x): data(x) {}
  // Temporarily, to allow creating a remote object from local data
  vector_t_component(const vector_t& x): data(boost::make_shared<vector_t>(x))
  {
  }
  // We don't really want these
  vector_t_component() { assert(0); __builtin_unreachable(); }
  
  vector_t::const_ptr get_data() const { return data; }
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, get_data);
  
  double get_elt(std::ptrdiff_t i) const { return (*data)(i); }
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, get_elt);
  void set_elt(std::ptrdiff_t i, double x) { (*data)(i) = x; }
  RPC_DEFINE_COMPONENT_ACTION(vector_t_component, set_elt);
  
  rpc::id_type faxpy(double alpha, const rpc::id_type& xi) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, faxpy);
  rpc::id_type fcopy() const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, fcopy);
  double fnrm2() const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, fnrm2);
  rpc::id_type fscal(double alpha) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, fscal);
  rpc::id_type fset(double alpha) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(vector_t_component, fset);
};

struct vector_t_client:
  rpc::components::client_base<vector_t_client,
                               rpc::components::stub_base<vector_t_component>>
{
  typedef
    rpc::components::client_base<vector_t_client,
                                 rpc::components::stub_base<vector_t_component>>
    client_base;
  vector_t_client() {}
  explicit vector_t_client(const rpc::id_type& gid): client_base(gid) {}
  explicit vector_t_client(const rpc::future<rpc::id_type>& fgid):
    client_base(fgid)
  {
  }
 // private:
 //  static auto wrap_get_gid(const rpc::future<vector_t_client>& fx) ->
 //    rpc::id_type
 //  {
 //    return fx.get().get_gid();
 //  }
 // public:
 //  explicit vector_t_client(rpc::future<vector_t_client> fx):
 //    client_base(fx.then(wrap_get_gid))
 //  {
 //  }
  explicit vector_t_client(const vector_t::ptr& x)
  {
    create(rpc::find_here(), x);
  }
  
  rpc::future<vector_t::const_ptr> get_data(bool want_data=true) const
  {
    if (!want_data) return rpc::make_ready_future(vector_t::const_ptr());
    return rpc::async(vector_t_component::get_data_action(), get_gid());
  }
  vector_t::const_ptr get_data_sync(bool want_data=true) const
  {
    if (!want_data) return vector_t::const_ptr();
    return vector_t_component::get_data_action()(get_gid());
  }
  
  double get_elt(std::ptrdiff_t i) const
  {
    return vector_t_component::get_elt_action()(get_gid(), i);
  }
  void set_elt(std::ptrdiff_t i, double x) const
  {
    return vector_t_component::set_elt_action()(get_gid(), i, x);
  }
  
  vector_t_client faxpy(double alpha, const vector_t_client& x) const
  {
    auto x_gid = alpha == 0.0 ? rpc::id_type() : x.get_gid();
    return vector_t_client(rpc::async(vector_t_component::faxpy_action(),
                                      get_gid(),
                                      alpha, x_gid));
  }
  vector_t_client fcopy() const
  {
    return vector_t_client(rpc::async(vector_t_component::fcopy_action(),
                                      get_gid()));
  }
  rpc::future<double> fnrm2() const
  {
    return rpc::async(vector_t_component::fnrm2_action(), get_gid());
  }
  vector_t_client fscal(double alpha) const
  {
    return vector_t_client(rpc::async(vector_t_component::fscal_action(),
                                      get_gid(),
                                      alpha));
  }
  vector_t_client fset(double alpha) const
  {
    return vector_t_client(rpc::async(vector_t_component::fset_action(),
                                      get_gid(),
                                      alpha));
  }
};



struct matrix_t_component:
  public rpc::components::simple_component_base<matrix_t_component>
{
  matrix_t::ptr data;
  
  matrix_t_component(std::ptrdiff_t NI, std::ptrdiff_t NJ):
    data(boost::make_shared<matrix_t>(NI,NJ))
  {
  }
  matrix_t_component& operator=(const matrix_t_component&) = delete;
  matrix_t_component(const matrix_t::ptr& a): data(a) {}
  // Temporarily, to allow creating a remote object from local data
  matrix_t_component(const matrix_t& a): data(boost::make_shared<matrix_t>(a))
  {
  }
  // We don't really want these
  matrix_t_component() { assert(0); __builtin_unreachable(); }
  
  matrix_t::const_ptr get_data() const { return data; }
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, get_data);
  
  double get_elt(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    return (*data)(i,j);
  }
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, get_elt);
  void set_elt(std::ptrdiff_t i, std::ptrdiff_t j, double x)
  {
    (*data)(i,j) = x;
  }
  RPC_DEFINE_COMPONENT_ACTION(matrix_t_component, set_elt);
  
  rpc::id_type faxpy(bool transa, bool transb0,
                     double alpha, const rpc::id_type& ai) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, faxpy);
  rpc::id_type fcopy(bool transa) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fcopy);
  rpc::id_type fgemm(bool transa, bool transb, bool transc0,
                     double alpha, const rpc::id_type& bi,
                     double beta, const rpc::id_type& ci0) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fgemm);
  rpc::id_type fgemv(bool trans, double alpha, const rpc::id_type& xi,
                     double beta, const rpc::id_type& yi0) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fgemv);
  double fnrm2() const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fnrm2);
  rpc::id_type fscal(bool trans, double alpha) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fscal);
  rpc::id_type fset(bool trans, double alpha) const;
  RPC_DEFINE_COMPONENT_CONST_ACTION(matrix_t_component, fset);
};

struct matrix_t_client:
  rpc::components::client_base<matrix_t_client,
                               rpc::components::stub_base<matrix_t_component>>
{
  typedef
    rpc::components::client_base<matrix_t_client,
                                 rpc::components::stub_base<matrix_t_component>>
    client_base;
  
  matrix_t_client() {}
  explicit matrix_t_client(const rpc::id_type& gid): client_base(gid) {}
  explicit matrix_t_client(const rpc::future<rpc::id_type>& fgid):
    client_base(fgid)
  {
  }
  explicit matrix_t_client(const matrix_t::ptr& x)
  {
    create(rpc::find_here(), x);
  }
  
  rpc::future<matrix_t::const_ptr> get_data(bool want_data=true) const
  {
    if (!want_data) return rpc::make_ready_future(matrix_t::const_ptr());
    return rpc::async(matrix_t_component::get_data_action(), get_gid());
  }
  matrix_t::const_ptr get_data_sync(bool want_data=true) const
  {
    if (!want_data) return matrix_t::const_ptr();
    return matrix_t_component::get_data_action()(get_gid());
  }
  
  double get_elt(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    return matrix_t_component::get_elt_action()(get_gid(), i, j);
  }
  void set_elt(std::ptrdiff_t i, std::ptrdiff_t j, double x) const
  {
    return matrix_t_component::set_elt_action()(get_gid(), i, j, x);
  }
  
  matrix_t_client faxpy(bool transa, bool transb0,
                        double alpha, const matrix_t_client& a) const
  {
    auto a_gid = alpha == 0.0 ? rpc::id_type() : a.get_gid();
    return matrix_t_client(rpc::async(matrix_t_component::faxpy_action(),
                                      get_gid(),
                                      transa, transb0, alpha, a_gid));
  }
  matrix_t_client fcopy(bool transa) const
  {
    return matrix_t_client(rpc::async(matrix_t_component::fcopy_action(),
                                      get_gid(),
                                      transa));
  }
  matrix_t_client fgemm(bool transa, bool transb, bool transc0,
                        double alpha, const matrix_t_client& b,
                        double beta, const matrix_t_client& c0) const
  {
    auto b_gid = alpha == 0.0 ? rpc::id_type() : b.get_gid();
    auto c0_gid = beta == 0.0 ? rpc::id_type() : c0.get_gid();
    return matrix_t_client(rpc::async(matrix_t_component::fgemm_action(),
                                      get_gid(),
                                      transa, transb, transc0,
                                      alpha, b_gid, beta, c0_gid));
  }
  vector_t_client fgemv(bool trans,
                        double alpha, const vector_t_client& x,
                        double beta, const vector_t_client& y0) const
  {
    auto x_gid = alpha == 0.0 ? rpc::id_type() : x.get_gid();
    auto y0_gid = beta == 0.0 ? rpc::id_type() : y0.get_gid();
    return vector_t_client(rpc::async(matrix_t_component::fgemv_action(),
                                      get_gid(),
                                      trans, alpha, x_gid, beta, y0_gid));
  }
  rpc::future<double> fnrm2() const
  {
    return rpc::async(matrix_t_component::fnrm2_action(), get_gid());
  }
  matrix_t_client fscal(bool trans, double alpha) const
  {
    return matrix_t_client(rpc::async(matrix_t_component::fscal_action(),
                                      get_gid(),
                                      trans, alpha));
  }
  matrix_t_client fset(bool trans, double alpha) const
  {
    return matrix_t_client(rpc::async(matrix_t_component::fset_action(),
                                      get_gid(),
                                      trans, alpha));
  }
};

#endif // #ifndef MATRIX_RPC_HH
