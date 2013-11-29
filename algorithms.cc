#include "algorithms.hh"

#include "matrix.hh"

#include <cassert>
#include <iostream>



#if defined CBLAS
#  if defined GSL_CBLAS
#    include <gsl_cblas.h>
#  elif defined MKL_CBLAS
#    include <mkl_cblas.h>
#  else
#    include <cblas.h>
#  endif
#endif

#if defined BLAS
extern "C" {
  void daxpy_(const int& N,
              const double& alpha,
              const double* __restrict__ x, const int& incx,
              double* __restrict__ y, const int& incy);
  void dcopy_(const int& N,
              const double* __restrict__ x, const int& incx,
              double* __restrict__ y, const int& incy);
  void dgemm_(const char& transa, const char& transb,
              const int& M, const int& N, const int& K,
              const double& alpha,
              const double* __restrict__ a, const int& lda,
              const double* __restrict__ b, const int& ldb,
              const double& beta,
              double* __restrict__ c, const int& ldc);
  void dgemv_(const char& trans,
              const int& M, const int& N,
              const double& alpha,
              const double* __restrict__ a, const int& lda,
              const double* __restrict__ x, const int& incx,
              const double& beta,
              double* __restrict__ y, const int& incy);
  double dnrm2_(const int& N,
                const double* __restrict__ x, const int& incx);
  void dscal_(const int& N,
              const double& alpha,
              double* __restrict__ x, const int& incx);
}
#endif



// Level 1

// axpy: y = alpha x + y

void axpy(double alpha, const vector_t& x, vector_t& y)
{
#if defined CBLAS
  if (alpha != 0.0) {
    assert(y.N == x.N);
  }
  cblas_daxpy(y.N, alpha, &x(0), 1, &y(0), 1);
#elif defined BLAS
  if (alpha != 0.0) {
    assert(y.N == x.N);
  }
  daxpy_(y.N, alpha, &x(0), 1, &y(0), 1);
#else
  if (alpha == 0.0) return;
  assert(y.N == x.N);
  if (alpha == 1.0) {
    double* __restrict__ const yp = &y(0);
    const double* __restrict__ const xp = &x(0);
    for (std::ptrdiff_t i=0; i<y.N; ++i) {
      // y(i) += x(i);
      yp[i] += xp[i];
    }
  } else {
    double* __restrict__ const yp = &y(0);
    const double* __restrict__ const xp = &x(0);
    for (std::ptrdiff_t i=0; i<y.N; ++i) {
      // y(i) += alpha * x(i);
      yp[i] += alpha * xp[i];
    }
  }
#endif
}



// copy: y = x

void copy(const vector_t& x, vector_t& y)
{
#if defined CBLAS
  assert(y.N == x.N);
  cblas_dcopy(y.N, &x(0), 1, &y(0), 1);
#elif defined BLAS
  assert(y.N == x.N);
  dcopy_(y.N, &x(0), 1, &y(0), 1);
#else
  assert(y.N == x.N);
  double* __restrict__ const yp = &y(0);
  const double* __restrict__ const xp = &x(0);
  for (std::ptrdiff_t i=0; i<y.N; ++i) {
    // y(i) = x(i);
    yp[i] = xp[i];
  }
#endif
}



// nrm2: sqrt(x^T x)

double nrm2(const vector_t& x)
{
#if defined CBLAS
  return cblas_dnrm2(x.N, &x(0), 1);
#elif defined BLAS
  return dnrm2_(x.N, &x(0), 1);
#else
  double val = nrm2_init();
  const double* __restrict__ const xp = &x(0);
  for (std::ptrdiff_t i=0; i<x.N; ++i) {
    // val = nrm2_combine(val, nrm2_process(x(i)));
    val = nrm2_combine(val, nrm2_process(xp[i]));
  }
  return nrm2_finalize(val);
#endif
}



// scal: x = alpha x

void scal(double alpha, vector_t& x)
{
#if defined CBLAS
  cblas_dscal(x.N, alpha, &x(0), 1);
#elif defined BLAS
  dscal_(x.N, alpha, &x(0), 1);
#else
  if (alpha == 1.0) return;
  if (alpha == 0.0) {
    double* __restrict__ const xp = &x(0);
    for (std::ptrdiff_t i=0; i<x.N; ++i) {
      // x(i) = 0.0;
      xp[i] = 0.0;
    }
  } else {
    double* __restrict__ const xp = &x(0);
    for (std::ptrdiff_t i=0; i<x.N; ++i) {
      // x(i) *= alpha;
      xp[i] *= alpha;
    }
  }
#endif
}



// set: x = alpha

void set(double alpha, vector_t& x)
{
  double* __restrict__ const xp = &x(0);
  for (std::ptrdiff_t i=0; i<x.N; ++i) {
    // x(i) = alpha;
    xp[i] = alpha;
  }
}



// Level 2

// axpy: b = alpha a + b

void axpy(bool transa, double alpha, const matrix_t& a, matrix_t& b)
{
#if defined CBLAS
  if (alpha != 0.0) {
    if (!transa) {
      assert(b.NI == a.NI);
      assert(b.NJ == a.NJ);
    } else {
      assert(b.NI == a.NJ);
      assert(b.NJ == a.NI);
    }
  }
  assert(!transa);
  cblas_daxpy(b.NI*b.NJ, alpha, &a(0,0), 1, &b(0,0), 1);
#elif defined BLAS
  if (alpha != 0.0) {
    if (!transa) {
      assert(b.NI == a.NI);
      assert(b.NJ == a.NJ);
    } else {
      assert(b.NI == a.NJ);
      assert(b.NJ == a.NI);
    }
  }
  assert(!transa);
  daxpy_(b.NI*b.NJ, alpha, &a(0,0), 1, &b(0,0), 1);
#else
  if (alpha == 0.0) return;
  if (!transa) {
    assert(b.NI == a.NI);
    assert(b.NJ == a.NJ);
    if (alpha == 1.0) {
      const double* __restrict__ const ap = &a(0,0);
      double* __restrict__ const bp = &b(0,0);
      for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<b.NI; ++i) {
          // b(i,j) += a(i,j);
          bp[i+b.NI*j] += ap[i+a.NI*j];
        }
      }
    } else {
      const double* __restrict__ const ap = &a(0,0);
      double* __restrict__ const bp = &b(0,0);
      for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<b.NI; ++i) {
          // b(i,j) += alpha * a(i,j);
          bp[i+b.NI*j] += alpha * ap[i+a.NI*j];
        }
      }
    }
  } else {
    assert(b.NI == a.NJ);
    assert(b.NJ == a.NI);
    if (alpha == 1.0) {
      const double* __restrict__ const ap = &a(0,0);
      double* __restrict__ const bp = &b(0,0);
      for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<b.NI; ++i) {
          // b(i,j) += a(j,i);
          bp[i+b.NI*j] += ap[j+a.NI*i];
        }
      }
    } else {
      const double* __restrict__ const ap = &a(0,0);
      double* __restrict__ const bp = &b(0,0);
      for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<b.NI; ++i) {
          // b(i,j) += alpha * a(j,i);
          bp[i+b.NI*j] += alpha * ap[j+a.NI*i];
        }
      }
    }
  }
#endif
}



// copy: y = x

void copy(bool transa, const matrix_t& a, matrix_t& b)
{
#if defined CBLAS
  if (!transa) {
    assert(b.NI == a.NI);
    assert(b.NJ == a.NJ);
  } else {
    assert(b.NI == a.NJ);
    assert(b.NJ == a.NI);
  }
  assert(!transa);
  cblas_dcopy(b.NI*b.NJ, &a(0,0), 1, &b(0,0), 1);
#elif defined BLAS
  if (!transa) {
    assert(b.NI == a.NI);
    assert(b.NJ == a.NJ);
  } else {
    assert(b.NI == a.NJ);
    assert(b.NJ == a.NI);
  }
  assert(!transa);
  dcopy_(b.NI*b.NJ, &a(0,0), 1, &b(0,0), 1);
#else
  if (!transa) {
    assert(b.NI == a.NI);
    assert(b.NJ == a.NJ);
    const double* __restrict__ const ap = &a(0,0);
    double* __restrict__ const bp = &b(0,0);
    for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
      for (std::ptrdiff_t i=0; i<b.NI; ++i) {
        // b(i,j) = a(i,j);
        bp[i+b.NI*j] = ap[i+a.NI*j];
      }
    }
  } else {
    assert(b.NI == a.NJ);
    assert(b.NJ == a.NI);
    const double* __restrict__ const ap = &a(0,0);
    double* __restrict__ const bp = &b(0,0);
    for (std::ptrdiff_t j=0; j<b.NJ; ++j) {
      for (std::ptrdiff_t i=0; i<b.NI; ++i) {
        // b(i,j) = a(j,i);
        bp[i+b.NI*j] = ap[j+a.NI*i];
      }
    }
  }
#endif
}



// gemv: y = alpha T[a] x + beta y

void gemv(bool trans, double alpha, const matrix_t& a, const vector_t& x,
          double beta, vector_t& y)
{
#ifndef CBLAS
  scal(beta, y);
  if (alpha == 0.0) return;
  if (!trans) {
    assert(a.NJ == x.N);
    assert(a.NI == y.N);
    const double* __restrict__ const ap = &a(0,0);
    const double* __restrict__ const xp = &x(0);
    double* __restrict__ const yp = &y(0);
    for (std::ptrdiff_t j=0; j<x.N; ++j) {
      // double tmp = alpha * x(j);
      double tmp = alpha * xp[j];
      for (std::ptrdiff_t i=0; i<y.N; ++i) {
        // y(i) += a(i,j) * tmp;
        yp[i] += ap[i+a.NI*j] * tmp;
      }
    }
  } else {
    assert(a.NI == x.N);
    assert(a.NJ == y.N);
    const double* __restrict__ const ap = &a(0,0);
    const double* __restrict__ const xp = &x(0);
    double* __restrict__ const yp = &y(0);
    for (std::ptrdiff_t j=0; j<y.N; ++j) {
      double tmp = 0.0;
      for (std::ptrdiff_t i=0; i<x.N; ++i) {
        // tmp += a(i,j) * x(i);
        tmp += ap[i+a.NI*j] * xp[i];
      }
      // y(j) += alpha * tmp;
      yp[j] += alpha * tmp;
    }
  }
#else
  if (alpha != 0.0) {
    if (!trans) {
      assert(a.NJ == x.N);
      assert(a.NI == y.N);
    } else {
      assert(a.NI == x.N);
      assert(a.NJ == y.N);
    }
  }
  cblas_dgemv(CblasColMajor, trans ? CblasTrans : CblasNoTrans,
              y.N, x.N, alpha, &a(0,0), a.NI, &x(0), 1, beta, &y(0), 1);
#endif
}



// nrm2: sqrt(trace a^T a)

double nrm2(const matrix_t& a)
{
#ifndef CBLAS
  double val = nrm2_init();
  const double* __restrict__ const ap = &a(0,0);
  for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
    for (std::ptrdiff_t i=0; i<a.NI; ++i) {
      // val = nrm2_combine(val, nrm2_process(a(i,j)));
      val = nrm2_combine(val, nrm2_process(ap[i+a.NI*j]));
    }
  }
  return nrm2_finalize(val);
#else
  return cblas_dnrm2(a.NI*a.NJ, &a(0,0), 1);
#endif
}



// scal: a = alpha a

void scal(double alpha, matrix_t& a)
{
#ifndef CBLAS
  if (alpha == 1.0) return;
  if (alpha == 0.0) {
    double* __restrict__ const ap = &a(0,0);
    for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
      for (std::ptrdiff_t i=0; i<a.NI; ++i) {
        // a(i,j) = 0.0;
        ap[i+a.NI*j] = 0.0;
      }
    }
  } else {
    double* __restrict__ const ap = &a(0,0);
    for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
      for (std::ptrdiff_t i=0; i<a.NI; ++i) {
        // a(i,j) *= alpha;
        ap[i+a.NI*j] *= alpha;
      }
    }
  }
#else
  cblas_dscal(a.NI*a.NJ, alpha, &a(0,0), 1);
#endif
}



// set: a = alpha

void set(double alpha, matrix_t& a)
{
  double* __restrict__ const ap = &a(0,0);
  for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
    for (std::ptrdiff_t i=0; i<a.NI; ++i) {
      // a(i,j) = alpha;
      ap[i+a.NI*j] = alpha;
    }
  }
}



// Level 3

// gemm: c = alpha T[a] T[b] + beta c

void gemm(bool transa, bool transb,
          double alpha, const matrix_t& a, const matrix_t& b,
          double beta, matrix_t& c)
{
#ifndef CBLAS
  if (alpha == 0.0) {
    scal(beta, c);
    return;
  }
  if (!transb) {
    if (!transa) {
      // c = alpha a b + beta c
      assert(b.NI == a.NJ);
      assert(c.NI == a.NI);
      assert(c.NJ == b.NJ);
      const double* __restrict__ const ap = &a(0,0);
      const double* __restrict__ const bp = &b(0,0);
      double* __restrict__ const cp = &c(0,0);
      for (std::ptrdiff_t j=0; j<c.NJ; ++j) {
        if (beta == 0.0) {
          for (std::ptrdiff_t i=0; i<c.NI; ++i) {
            // c(i,j) = 0.0;
            cp[i+c.NI*j] = 0.0;
          }
        } else {
          for (std::ptrdiff_t i=0; i<c.NI; ++i) {
            // c(i,j) *= beta;
            cp[i+c.NI*j] *= beta;
          }
        }
        for (std::ptrdiff_t k=0; k<b.NI; ++k) {
          // if (b(k,j) != 0.0) {
          if (bp[k+b.NI*j] != 0.0) {
            // double tmp = alpha * b(k,j);
            double tmp = alpha * bp[k+b.NI*j];
            for (std::ptrdiff_t i=0; i<c.NI; ++i) {
              // c(i,j) += tmp * a(i,k);
              cp[i+c.NI*j] += tmp * ap[i+a.NI*k];
            }
          }
        }
      }
    } else {
      // c = alpha a^T b + beta c
      assert(b.NI == a.NI);
      assert(c.NI == a.NJ);
      assert(c.NJ == b.NJ);
      const double* __restrict__ const ap = &a(0,0);
      const double* __restrict__ const bp = &b(0,0);
      double* __restrict__ const cp = &c(0,0);
      for (std::ptrdiff_t j=0; j<c.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<c.NI; ++i) {
          double tmp = 0.0;
          for (std::ptrdiff_t k=0; k<b.NI; ++k) {
            tmp += ap[k+a.NI*i] * bp[k+b.NI*j];
          }
          if (beta == 0.0) {
            cp[i+c.NI*j] = alpha * tmp;
          } else {
            cp[i+c.NI*j] = alpha * tmp + beta * cp[i+c.NI*j];
          }
        }
      }
    }
  } else {
    if (!transa) {
      // c = alpha a b^T + beta c
      assert(b.NJ == a.NJ);
      assert(c.NI == a.NI);
      assert(c.NJ == b.NI);
      const double* __restrict__ const ap = &a(0,0);
      const double* __restrict__ const bp = &b(0,0);
      double* __restrict__ const cp = &c(0,0);
      for (std::ptrdiff_t j=0; j<c.NJ; ++j) {
        if (beta == 0.0) {
          for (std::ptrdiff_t i=0; i<c.NI; ++i) {
            cp[i+c.NI*j] = 0.0;
          }
        } else if (beta != 1.0) {
          for (std::ptrdiff_t i=0; i<c.NI; ++i) {
            cp[i+c.NI*j] *= beta;
          }
        }
        for (std::ptrdiff_t k=0; k<b.NJ; ++k) {
          if (bp[j+b.NI*k] != 0.0) {
            double tmp = alpha * bp[j+b.NI*k];
            for (std::ptrdiff_t i=0; i<c.NI; ++i) {
              cp[i+c.NI*j] += tmp * ap[i+a.NI*k];
            }
          }
        }
      }
    } else {
      // c = alpha a^T b^T + beta c
      assert(b.NJ == a.NI);
      assert(c.NI == a.NJ);
      assert(c.NJ == b.NI);
      const double* __restrict__ const ap = &a(0,0);
      const double* __restrict__ const bp = &b(0,0);
      double* __restrict__ const cp = &c(0,0);
      for (std::ptrdiff_t j=0; j<c.NJ; ++j) {
        for (std::ptrdiff_t i=0; i<c.NI; ++i) {
          double tmp = 0.0;
          for (std::ptrdiff_t k=0; k<b.NJ; ++k) {
            tmp += ap[k+a.NI*i] * bp[j+b.NI*k];
          }
          if (beta == 0.0) {
            cp[i+c.NI*j] = alpha * tmp;
          } else {
            cp[i+c.NI*j] = alpha * tmp + beta * cp[i+c.NI*j];
          }
        }
      }
    }
  }
#else
  if (alpha != 0.0) {
    if (!transb) {
      if (!transa) {
        // c = alpha a b + beta c
        assert(b.NI == a.NJ);
        assert(c.NI == a.NI);
        assert(c.NJ == b.NJ);
      } else {
        // c = alpha a^T b + beta c
        assert(b.NI == a.NI);
        assert(c.NI == a.NJ);
        assert(c.NJ == b.NJ);
      }
    } else {
      if (!transa) {
        // c = alpha a b^T + beta c
        assert(b.NJ == a.NJ);
        assert(c.NI == a.NI);
        assert(c.NJ == b.NI);
      } else {
        // c = alpha a^T b^T + beta c
        assert(b.NJ == a.NI);
        assert(c.NI == a.NJ);
        assert(c.NJ == b.NI);
      }
    }
  }
  cblas_dgemm(CblasColMajor,
              transa ? CblasTrans : CblasNoTrans,
              transb ? CblasTrans : CblasNoTrans,
              c.NI, c.NJ, a.NJ,
              alpha, &a(0,0), a.NI, &b(0,0), b.NI, beta, &c(0,0), c.NI);
#endif
}
