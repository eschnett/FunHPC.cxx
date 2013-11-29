#include "matrix_hpx.hh"

#include "algorithms.hh"
#include "algorithms_fun.hh"



HPX_REGISTER_COMPONENT_MODULE();



HPX_REGISTER_MINIMAL_COMPONENT_FACTORY
(hpx::components::simple_component<vector_t_component>, vector_t_factory);

hpx::id_type vector_t_component::faxpy(double alpha, const hpx::id_type& xi)
  const
{
  vector_t_client x(xi);
  auto x_ptr = x.get_data_sync(alpha != 0.0);
  auto y = ::faxpy(alpha, x_ptr, data);
  return vector_t_client(y).get_gid();
}
hpx::id_type vector_t_component::fcopy() const
{
  auto y = ::fcopy(data);
  return vector_t_client(y).get_gid();
}
double vector_t_component::fnrm2() const
{
  return ::fnrm2(data);
}
hpx::id_type vector_t_component::fscal(double alpha) const
{
  auto y = ::fscal(alpha, data);
  return vector_t_client(y).get_gid();
}
hpx::id_type vector_t_component::fset(double alpha) const
{
  auto y = ::fset(alpha, data);
  return vector_t_client(y).get_gid();
}



HPX_REGISTER_MINIMAL_COMPONENT_FACTORY
(hpx::components::simple_component<matrix_t_component>, matrix_t_factory);

hpx::id_type matrix_t_component::faxpy(bool transa, bool transb,
                                       double alpha, const hpx::id_type& ai)
  const
{
  matrix_t_client a(ai);
  auto a_ptr = a.get_data_sync(alpha != 0.0);
  auto b = ::faxpy(transa, transb, alpha, a_ptr, data);
  return matrix_t_client(b).get_gid();
}
hpx::id_type matrix_t_component::fcopy(bool transa) const
{
  auto b = ::fcopy(transa, data);
  return matrix_t_client(b).get_gid();
}
hpx::id_type matrix_t_component::fgemm(bool transa, bool transb, bool transc0,
                                       double alpha, const hpx::id_type& bi,
                                       double beta, const hpx::id_type& ci0)
  const
{
  assert(alpha != 0.0);
  matrix_t_client b(bi), c0(ci0);
  auto b_ptr = b.get_data_sync();
  auto c0_ptr = c0.get_data_sync(beta != 0.0);
  auto c = ::fgemm(transa, transb, transc0, alpha, data, b_ptr, beta, c0_ptr);
  return matrix_t_client(c).get_gid();
}
hpx::id_type matrix_t_component::fgemv(bool trans,
                                       double alpha, const hpx::id_type& xi,
                                       double beta, const hpx::id_type& yi0)
  const
{
  vector_t_client x(xi), y0(yi0);
  auto x_ptr = x.get_data_sync(alpha != 0.0);
  auto y0_ptr = y0.get_data_sync(beta != 0.0);
  auto y = ::fgemv(trans, alpha, data, x_ptr, beta, y0_ptr);
  return vector_t_client(y).get_gid();
}
double matrix_t_component::fnrm2() const
{
  return ::fnrm2(data);
}
hpx::id_type matrix_t_component::fscal(bool trans, double alpha) const
{
  auto a = ::fscal(trans, alpha, data);
  return matrix_t_client(a).get_gid();
}
hpx::id_type matrix_t_component::fset(bool trans, double alpha) const
{
  auto a = ::fset(trans, alpha, data);
  return matrix_t_client(a).get_gid();
}
