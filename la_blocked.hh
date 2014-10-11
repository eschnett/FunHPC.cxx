#ifndef LA_BLOCKED_HH
#define LA_BLOCKED_HH

#include "rpc.hh"

#include "la_dense.hh"

#include <cassert>
#include <vector>

namespace la {
using namespace rpc;
using namespace std;

struct blocked_vector_t;
struct blocked_matrix_t;

////////////////////////////////////////////////////////////////////////////////

struct blocked_vector_t {

  vector<client<vector_t> > blocks;

  blocked_vector_t(ptrdiff_t nb) : blocks(nb) {}

  void set(ptrdiff_t i, const client<vector_t> &block) {
    assert(i >= 0 && i < size());
    blocks[i] = block;
  }

  const client<vector_t> &operator()(ptrdiff_t i) const {
    assert(i >= 0 && i < nblocks());
    return blocks[i];
  }

  ptrdiff_t nblocks() const { return blocks.size(); }
  ptrdiff_t size() const;
};

// Level 1

blocked_vector_t addvb(const blocked_vector_t &x, const blocked_vector_t &y);

blocked_vector_t copyvb(const blocked_vector_t &x);

double dotvb(const blocked_vector_t &x, const blocked_vector_t &y);

double fnormvb(const blocked_vector_t &x);

blocked_vector_t invertvb(const vector_t &x);

blocked_vector_t scalvb(const blocked_vector_t &x, double alpha);

blocked_vector_t setvb(ptrdiff_t n, ptrdiff_t nb, double alpha);

// Note: Calculates y - x
blocked_vector_t subvb(const blocked_vector_t &x, const blocked_vector_t &y);

// Level 2

blocked_vector_t gemvb(const blocked_matrix_t &a, const blocked_vector_t &x,
                       bool transa, double alpha);

////////////////////////////////////////////////////////////////////////////////

struct blocked_matrix_t {
  ptrdiff_t nrowblocks_;
  vector<client<matrix_t> > blocks;

  blocked_matrix_t(ptrdiff_t m, ptrdiff_t n) : nrowblocks_(m), blocks(m * n) {}

  void set(ptrdiff_t i, ptrdiff_t j, const client<matrix_t> &block) {
    assert(i >= 0 && i < nrowblocks());
    assert(j >= 0 && j < ncolblocks());
    blocks[i + nrowblocks_ * j] = block;
  }

  const client<matrix_t> &operator()(ptrdiff_t i, ptrdiff_t j,
                                     bool trans = false) const {
    if (trans)
      return (*this)(j, i);
    assert(i >= 0 && i < nrowblocks());
    assert(j >= 0 && j < ncolblocks());
    assert(i + nrowblocks_ * j < nblocks());
    return blocks[i + nrowblocks_ * j];
  }

  ptrdiff_t nblocks() const { return blocks.size(); }
  ptrdiff_t nrowblocks(bool trans = false) const {
    if (trans)
      return ncolblocks();
    return nrowblocks_;
  }
  ptrdiff_t ncolblocks(bool trans = false) const {
    if (trans)
      return nrowblocks();
    return nblocks() / nrowblocks();
  }

  ptrdiff_t size() const;
  // ptridff_t nrows() const;
  // ptridff_t ncols() const;
};

// Level 1

blocked_matrix_t addmb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa, bool transb);

blocked_matrix_t copymb(const blocked_matrix_t &a, bool transa);

double fnormmb(const blocked_matrix_t &a);

blocked_matrix_t scalmb(const blocked_matrix_t &a, bool transa, double alpha);

blocked_matrix_t setmb(ptrdiff_t m, ptrdiff_t n, double alpha);

// Note: Calculates y - x
blocked_matrix_t submb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa);

// Level 2

blocked_matrix_t gerb(const blocked_vector_t &x, const blocked_vector_t &y,
                      double alpha);

// Level 3

blocked_matrix_t gemmb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa, bool transb, double alpha);
}

#endif // #ifndef LA_BLOCKED_HH
