#ifndef LA_DENSE_HH
#define LA_DENSE_HH

#include "rpc.hh"

#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <tuple>
#include <vector>

namespace la {
using namespace rpc;
using namespace std;

class vector_t;
class matrix_t;

////////////////////////////////////////////////////////////////////////////////

class vector_t {
  vector<double> elts;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(elts); }

public:
  vector_t() {} // only for serialization

  vector_t(ptrdiff_t n) : elts(n) {}

  void set(ptrdiff_t i, double elt) {
    assert(i >= 0 && i < size());
    elts[i] = elt;
  }

  double operator()(ptrdiff_t i) const {
    assert(i >= 0 && i < size());
    return elts[i];
  }

  ptrdiff_t size() const { return elts.size(); }
};
RPC_DECLARE_COMPONENT(vector_t);

// Level 1

vector_t addv(const vector_t &x, const vector_t &y);
RPC_DECLARE_ACTION(addv);

vector_t copyv(const vector_t &x);
RPC_DECLARE_ACTION(copyv);

double dotv(const vector_t &x, const vector_t &y);
RPC_DECLARE_ACTION(dotv);

double fnormv(const vector_t &x);
RPC_DECLARE_ACTION(fnormv);

vector_t invertv(const vector_t &x);
RPC_DECLARE_ACTION(invertv);

vector_t scalv(const vector_t &x, double alpha);
RPC_DECLARE_ACTION(scalv);

vector_t setv(ptrdiff_t n, double alpha);
RPC_DECLARE_ACTION(setv);

// Note: Calculates y - x
vector_t subv(const vector_t &x, const vector_t &y);
RPC_DECLARE_ACTION(subv);

// Level 2

vector_t gemv(const matrix_t &a, const vector_t &x, bool transa, double alpha);
RPC_DECLARE_ACTION(gemv);

////////////////////////////////////////////////////////////////////////////////

class matrix_t {
  ptrdiff_t nrows_;
  vector<double> elts;

public:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(nrows_, elts); }

public:
  matrix_t() {} // only for serialization

  matrix_t(ptrdiff_t m, ptrdiff_t n) : nrows_(m), elts(m * n) {}

  void set(ptrdiff_t i, ptrdiff_t j, double elt) {
    assert(i >= 0 && i < nrows());
    assert(j >= 0 && j < ncols());
    assert(i + nrows_ * j < size());
    elts[i + nrows_ * j] = elt;
  }

  double operator()(ptrdiff_t i, ptrdiff_t j, bool trans = false) const {
    if (trans)
      return (*this)(j, i);
    assert(i >= 0 && i < nrows());
    assert(j >= 0 && j < ncols());
    assert(i + nrows_ * j < size());
    return elts[i + nrows_ * j];
  }

  ptrdiff_t size() const { return elts.size(); }
  ptrdiff_t nrows(bool trans = false) const {
    if (trans)
      return ncols();
    return nrows_;
  }
  ptrdiff_t ncols(bool trans = false) const {
    if (trans)
      return nrows();
    return size() / nrows();
  }
};
RPC_DECLARE_COMPONENT(matrix_t);

// Level 1

matrix_t addm(const matrix_t &a, const matrix_t &b, bool transa, bool transb);
RPC_DECLARE_ACTION(addm);

matrix_t copym(const matrix_t &a, bool transa);
RPC_DECLARE_ACTION(copym);

double fnormm(const matrix_t &a);
RPC_DECLARE_ACTION(fnormm);

matrix_t scalm(const matrix_t &a, bool transa, double alpha);
RPC_DECLARE_ACTION(scalm);

matrix_t setm(ptrdiff_t m, ptrdiff_t n, double alpha);
RPC_DECLARE_ACTION(setm);

// Note: Calculates y - x
matrix_t subm(const matrix_t &a, const matrix_t &b, bool transa, bool transb);
RPC_DECLARE_ACTION(subm);

// Level 2

matrix_t ger(const vector_t &x, const vector_t &y, double alpha);
RPC_DECLARE_ACTION(ger);

// Level 3

matrix_t gemm(const matrix_t &a, const matrix_t &b, bool transa, bool transb,
              double alpha);
RPC_DECLARE_ACTION(gemm);

////////////////////////////////////////////////////////////////////////////////
}

#endif // #ifndef LA_DENSE_HH
