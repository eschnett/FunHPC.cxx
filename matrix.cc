#include "matrix.hh"

#include "algorithms.hh"



RPC_IMPLEMENT_COMPONENT(vector_t);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, get_elt);
RPC_IMPLEMENT_MEMBER_ACTION(vector_t, set_elt);

std::ostream& operator<<(std::ostream& os, const vector_t& x)
{
  os << "[";
  for (std::ptrdiff_t i=0; i<x.N; ++i) {
    if (i != 0) os << ",";
    os << x(i);
  }
  os << "]";
  return os;
}

// Level 1

auto vector_t::faxpy(double alpha, const const_ptr& x) const -> ptr
{
  auto y = boost::make_shared<vector_t>(N);
  copy(*this, *y);
  axpy(alpha, *x, *y);
  return y;
}

auto vector_t::fcopy() const -> ptr
{
  auto y = boost::make_shared<vector_t>(N);
  copy(*this, *y);
  return y;
}

auto vector_t::fnrm2() const -> double
{
  return nrm2(*this);
}

auto vector_t::fscal(double alpha) const -> ptr
{
  auto x = boost::make_shared<vector_t>(N);
  if (alpha != 0.0) copy(*this, *x);
  scal(alpha, *x);
  return x;
}

auto vector_t::fset(double alpha) const -> ptr
{
  auto x = boost::make_shared<vector_t>(N);
  set(alpha, *x);
  return x;
}

RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, cfaxpy);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, cfcopy);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, cfnrm2);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, cfscal);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(vector_t, cfset);



RPC_IMPLEMENT_COMPONENT(matrix_t);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, get_elt);
RPC_IMPLEMENT_MEMBER_ACTION(matrix_t, set_elt);

std::ostream& operator<<(std::ostream& os, const matrix_t& a)
{
  os << "[";
  for (std::ptrdiff_t i=0; i<a.NI; ++i) {
    if (i != 0) os << ",";
    os << "[";
    for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
      if (j != 0) os << ",";
      os << a(i,j);
    }
    os << "]";
  }
  os << "]";
  return os;
}

// Level 2

auto matrix_t::faxpy(bool transa, bool transb0,
                     double alpha, const const_ptr& a) const -> ptr
{
  auto nib0 = !transb0 ? NI : NJ;
  auto njb0 = !transb0 ? NJ : NI;
  auto b = boost::make_shared<matrix_t>(nib0, njb0);
  copy(transb0, *this, *b);
  axpy(transa, alpha, *a, *b);
  return b;
}

auto matrix_t::fcopy(bool trans) const -> ptr
{
  auto nib = !trans ? NI : NJ;
  auto njb = !trans ? NJ : NI;
  auto b = boost::make_shared<matrix_t>(nib, njb);
  copy(trans, *this, *b);
  return b;
}

auto matrix_t::fgemv(bool trans,
                     double alpha, const vector_t::const_ptr& x,
                     double beta, const vector_t::const_ptr& y0) const ->
  vector_t::ptr
{
  if (alpha == 0.0) return y0->fscal(beta);
  auto y = boost::make_shared<vector_t>(NI);
  if (beta != 0.0) copy(*y0, *y);
  gemv(trans, alpha, *this, *x, beta, *y);
  return y;
}

auto matrix_t::fnrm2() const -> double
{
  return nrm2(*this);
}

auto matrix_t::fscal(bool trans, double alpha) const ->
  matrix_t::ptr
{
  auto nia0 = !trans ? NI : NJ;
  auto nja0 = !trans ? NJ : NI;
  auto a = boost::make_shared<matrix_t>(nia0, nja0);
  if (alpha != 0.0) copy(trans, *this, *a);
  scal(alpha, *a);
  return a;
}

auto matrix_t::fset(bool trans, double alpha) const -> ptr
{
  auto nia0 = !trans ? NI : NJ;
  auto nja0 = !trans ? NJ : NI;
  auto a = boost::make_shared<matrix_t>(nia0, nja0);
  set(alpha, *a);
  return a;
}

// Level 3

auto matrix_t::fgemm(bool transa, bool transb, bool transc0,
                     double alpha, const const_ptr& a, const const_ptr& b,
                     double beta) const -> ptr
{
  if (alpha == 0.0) return fscal(transc0, beta);
  auto nia = !transa ? a->NI : a->NJ;
  auto nja = !transa ? a->NJ : a->NI;
  auto nib = !transb ? b->NI : b->NJ;
  auto njb = !transb ? b->NJ : b->NI;
  auto nic0 = beta == 0.0 ? nia : !transc0 ? NI : NJ;
  auto njc0 = beta == 0.0 ? njb : !transc0 ? NJ : NI;
  assert(nib == nja);
  assert(nic0 == nia);
  assert(njc0 == njb);
  auto c = boost::make_shared<matrix_t>(nic0, njc0);
  if (beta != 0.0) copy(transc0, *this, *c);
  gemm(transa, transb, alpha, *a, *b, beta, *c);
  return c;
}

RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfaxpy);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfgemv);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfcopy);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfnrm2);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfscal);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfset);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(matrix_t, cfgemm);
