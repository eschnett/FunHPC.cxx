#ifndef ALGORITHMS_HH
#define ALGORITHMS_HH

#include "matrix.hh"

#include <cmath>



// Level 1

// axpy: y = alpha x + y
void axpy(double alpha, const vector_t& x, vector_t& y);

// copy: y = x
void copy(const vector_t& x, vector_t& y);

// nrm2: sqrt(x^T x)
inline double nrm2_init() { return 0.0; }
inline double nrm2_process(double xi) { return xi * xi; }
inline double nrm2_combine(double val0, double val1) { return val0 + val1; }
inline double nrm2_finalize(double val) { return std::sqrt(val); }
double nrm2(const vector_t& x);

// scal: x = alpha x
void scal(double alpha, vector_t& x);

// set: x = alpha
void set(double alpha, vector_t& x);



// Level 2

// axpy: b = alpha a + b
void axpy(bool transa, double alpha, const matrix_t& a, matrix_t& b);

// copy: b = a
void copy(bool transa, const matrix_t& a, matrix_t& b);

// gemv: y = alpha T[a] x + beta y
void gemv(bool trans, double alpha, const matrix_t& a, const vector_t& x,
          double beta, vector_t& y);

// nrm2: sqrt(trace a^T a)
double nrm2(const matrix_t& a);

// scal: a = alpha a
void scal(double alpha, matrix_t& a);

// set: a = alpha
void set(double alpha, matrix_t& a);



// Level 3

// gemm: c = alpha T[a] T[b] + beta c
void gemm(bool transa, bool transb,
          double alpha, const matrix_t& a, const matrix_t& b,
          double beta, matrix_t& c);

#endif  // #ifndef ALGORITHMS_HH
