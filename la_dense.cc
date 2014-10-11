#include "la_dense.hh"

namespace la {
using namespace rpc;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

// Level 1

vector_t addv(const vector_t &x, const vector_t &y) {
  ptrdiff_t n = y.size();
  assert(x.size() == n);
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, x(i) + y(i));
  return r;
}

vector_t axpyv(double alpha, const vector_t &x, const vector_t &y) {
  ptrdiff_t n = y.size();
  assert(x.size() == n);
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, alpha * x(i) + y(i));
  return r;
}

vector_t copyv(const vector_t &x) {
  ptrdiff_t n = x.size();
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, x(i));
  return r;
}

double dotv(const vector_t &x, const vector_t &y) {
  ptrdiff_t n = x.size();
  assert(y.size() == n);
  double res = 0.0;
  for (ptrdiff_t i = 0; i < n; ++i)
    res += x(i) * y(i);
  return res;
}

double fnormv(const vector_t &x) {
  ptrdiff_t n = x.size();
  double res = 0.0;
  for (ptrdiff_t i = 0; i < n; ++i)
    res += pow(x(i), 2);
  return sqrt(res);
}

vector_t invertv(const vector_t &x) {
  ptrdiff_t n = x.size();
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, 1.0 / x(i));
  return r;
}

vector_t scalv(const vector_t &x, double alpha) {
  ptrdiff_t n = x.size();
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, alpha * x(i));
  return r;
}

vector_t setv(ptrdiff_t n, double alpha) {
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, alpha);
  return r;
}

vector_t subv(const vector_t &x, const vector_t &y) {
  ptrdiff_t n = y.size();
  assert(x.size() == n);
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    // Note order of arguments
    r.set(i, y(i) - x(i));
  return r;
}

// Level 2

vector_t gemv(const matrix_t &a, const vector_t &x, bool transa, double alpha) {
  ptrdiff_t m = a.ncols(transa);
  ptrdiff_t n = a.nrows(transa);
  assert(x.size() == m);
  vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i) {
    double res = 0.0;
    for (ptrdiff_t j = 0; j < m; ++j)
      res += a(i, j, transa) * x(j);
    res *= alpha;
    r.set(i, res);
  }
  return r;
}

////////////////////////////////////////////////////////////////////////////////

// Level 1

matrix_t addm(const matrix_t &a, const matrix_t &b, bool transa, bool transb) {
  ptrdiff_t m = a.nrows(transa);
  ptrdiff_t n = a.ncols(transa);
  assert(b.nrows(transb) == m);
  assert(b.ncols(transb) == n);
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, a(i, j, transa) + b(i, j, transb));
  return r;
}

matrix_t copym(const matrix_t &a, bool transa) {
  ptrdiff_t m = a.nrows(transa);
  ptrdiff_t n = a.ncols(transa);
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, a(i, j, transa));
  return r;
}

double fnormm(const matrix_t &a) {
  double res = 0.0;
  ptrdiff_t m = a.nrows();
  ptrdiff_t n = a.ncols();
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      res += pow(a(i, j), 2);
  return sqrt(res);
}

matrix_t scalm(const matrix_t &a, bool transa, double alpha) {
  ptrdiff_t m = a.nrows(transa);
  ptrdiff_t n = a.ncols(transa);
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, alpha * a(i, j, transa));
  return r;
}

matrix_t setm(ptrdiff_t m, ptrdiff_t n, double alpha) {
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, alpha);
  return r;
}

matrix_t subm(const matrix_t &a, const matrix_t &b, bool transa, bool transb) {
  ptrdiff_t m = a.nrows(transa);
  ptrdiff_t n = a.ncols(transa);
  assert(b.nrows(transb) == m);
  assert(b.ncols(transb) == n);
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      // Note order of arguments
      r.set(i, j, b(i, j) - a(i, j, transa));
  return r;
}

// Level 2

matrix_t ger(const vector_t &x, const vector_t &y, double alpha) {
  ptrdiff_t m = x.size();
  ptrdiff_t n = y.size();
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      // Note order of arguments!
      r.set(i, j, alpha * x(i) * y(j));
  return r;
}

// Level 3

matrix_t gemm(const matrix_t &a, const matrix_t &b, bool transa, bool transb,
              double alpha) {
  ptrdiff_t m = a.nrows(transa);
  ptrdiff_t l = a.ncols(transa);
  ptrdiff_t n = b.ncols(transb);
  assert(b.nrows(transb) == l);
  matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j) {
    for (ptrdiff_t i = 0; i < m; ++i) {
      double res = 0.0;
      for (ptrdiff_t k = 0; k < l; ++k)
        res += a(i, k, transa) * b(k, j, transb);
      res *= alpha;
      r.set(i, j, res);
    }
  }
  return r;
}

////////////////////////////////////////////////////////////////////////////////
}

RPC_IMPLEMENT_COMPONENT(la::vector_t);

RPC_IMPLEMENT_ACTION(la::addv);
RPC_IMPLEMENT_ACTION(la::copyv);
RPC_IMPLEMENT_ACTION(la::dotv);
RPC_IMPLEMENT_ACTION(la::fnormv);
RPC_IMPLEMENT_ACTION(la::invertv);
RPC_IMPLEMENT_ACTION(la::scalv);
RPC_IMPLEMENT_ACTION(la::setv);
RPC_IMPLEMENT_ACTION(la::subv);
RPC_IMPLEMENT_ACTION(la::gemv);

RPC_IMPLEMENT_COMPONENT(la::matrix_t);

RPC_IMPLEMENT_ACTION(la::addm);
RPC_IMPLEMENT_ACTION(la::copym);
RPC_IMPLEMENT_ACTION(la::fnormm);
RPC_IMPLEMENT_ACTION(la::scalm);
RPC_IMPLEMENT_ACTION(la::setm);
RPC_IMPLEMENT_ACTION(la::subm);
RPC_IMPLEMENT_ACTION(la::ger);
RPC_IMPLEMENT_ACTION(la::gemm);
