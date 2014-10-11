#include "la_blocked.hh"

#include "cxx_foldable.hh"
#include "cxx_functor.hh"

#include <cmath>

namespace la {
using namespace cxx;
using namespace rpc;
using namespace std;

ptrdiff_t blocked_vector_t::size() const {
  return foldl(
      [](ptrdiff_t s, const client<vector_t> &x) { return s + x->size(); },
      ptrdiff_t(0), blocks);
}

// Level 1

blocked_vector_t addvb(const blocked_vector_t &x, const blocked_vector_t &y) {
  ptrdiff_t n = y.nblocks();
  assert(x.nblocks() == n);
  blocked_vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, fmap2(addv_action(), x(i), y(i)));
  return r;
}

blocked_vector_t copyvb(const blocked_vector_t &x) {
  ptrdiff_t n = x.nblocks();
  blocked_vector_t r(n);
  // Make a shallow copy
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, x(i));
  return r;
}

double dotvb(const blocked_vector_t &x, const blocked_vector_t &y) {
  ptrdiff_t n = x.nblocks();
  assert(y.nblocks() == n);
  vector<client<double> > ress(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    ress[i] = fmap2(dotv_action(), x(i), y(i)).make_local();
  double res = foldl([](double s, const client<double> &r) { return s + *r; },
                     0.0, ress);
  return res;
}

double fnormvb(const blocked_vector_t &x) {
  ptrdiff_t n = x.nblocks();
  vector<client<double> > ress(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    ress[i] = fmap(fnormv_action(), x(i)).make_local();
  double res =
      foldl([](double s, const client<double> &r) { return s + pow(*r, 2); },
            0.0, ress);
  return sqrt(res);
}

blocked_vector_t invertvb(const blocked_vector_t &x) {
  ptrdiff_t n = x.nblocks();
  blocked_vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, fmap(invertv_action(), x(i)));
  return r;
}

blocked_vector_t scalvb(const blocked_vector_t &x, double alpha) {
  ptrdiff_t n = x.nblocks();
  blocked_vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, fmap(scalv_action(), x(i), alpha));
  return r;
}

blocked_vector_t setvb(ptrdiff_t n, ptrdiff_t nb, double alpha) {
  blocked_vector_t r(nb);
  // TODO: Use make_remote_client
  for (ptrdiff_t i = 0; i < nb; ++i)
    // r.set(i, make_client<vector_t>(setv_action(), n / nb, alpha));
    r.set(i, make_client<vector_t>(setv(n / nb, alpha)));
  return r;
}

blocked_vector_t subvb(const blocked_vector_t &x, const blocked_vector_t &y) {
  ptrdiff_t n = y.nblocks();
  assert(x.nblocks() == n);
  blocked_vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i)
    r.set(i, fmap2(subv_action(), x(i), y(i)));
  return r;
}

// Level 2

blocked_vector_t gemvb(const blocked_matrix_t &a, const blocked_vector_t &x,
                       bool transa, double alpha) {
  ptrdiff_t m = a.ncolblocks(transa);
  ptrdiff_t n = a.nrowblocks(transa);
  assert(x.nblocks() == m);
  blocked_vector_t r(n);
  for (ptrdiff_t i = 0; i < n; ++i) {
    vector<client<vector_t> > ress(n);
    for (ptrdiff_t j = 0; j < m; ++j)
      ress[n] = fmap2(gemv_action(), a(i, j, transa), x(j), transa, alpha);
    client<vector_t> res =
        foldl1([](const client<vector_t> &x, const client<vector_t> &y) {
                 return fmap2(addv_action(), x, y);
               },
               ress);
    r.set(i, res);
  }
  return r;
}

////////////////////////////////////////////////////////////////////////////////

// Level 1

blocked_matrix_t addmb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa, bool transb) {
  ptrdiff_t m = a.nrowblocks(transa);
  ptrdiff_t n = a.ncolblocks(transa);
  assert(b.nrowblocks(transb) == m);
  assert(b.ncolblocks(transb) == n);
  blocked_matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, fmap2(addm_action(), a(i, j, transa), b(i, j, transb), transa,
                        transb));
  return r;
}

blocked_matrix_t copymb(const blocked_matrix_t &a, bool transa) {
  ptrdiff_t m = a.nrowblocks(transa);
  ptrdiff_t n = a.ncolblocks(transa);
  blocked_matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j) {
    for (ptrdiff_t i = 0; i < m; ++i) {
      if (!transa) {
        // Make a shallow copy
        r.set(i, j, a(i, j));
      } else {
        r.set(i, j, fmap(copym_action(), a(i, j, transa), transa));
      }
    }
  }
  return r;
}

double fnormmb(const blocked_matrix_t &a) {
  ptrdiff_t m = a.nrowblocks();
  ptrdiff_t n = a.ncolblocks();
  vector<client<double> > ress(m * n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      ress[i + m * j] = fmap(fnormm_action(), a(i, j)).make_local();
  double res = foldl([](double s, const client<double> &r) { return s + *r; },
                     0.0, ress);
  return res;
}

blocked_matrix_t scalmb(const blocked_matrix_t &a, bool transa, double alpha) {
  ptrdiff_t m = a.nrowblocks(transa);
  ptrdiff_t n = a.ncolblocks(transa);
  blocked_matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, fmap(scalm_action(), a(i, j, transa), transa, alpha));
  return r;
}

blocked_matrix_t setmb(ptrdiff_t m, ptrdiff_t n, ptrdiff_t mb, ptrdiff_t nb,
                       double alpha) {
  blocked_matrix_t r(mb, nb);
  // TODO: Use make_remote_client
  for (ptrdiff_t j = 0; j < nb; ++j)
    for (ptrdiff_t i = 0; i < mb; ++i)
      r.set(i, j, make_client<matrix_t>(setm(m / mb, n / nb, alpha)));
  return r;
}

blocked_matrix_t submb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa, bool transb) {
  ptrdiff_t m = a.nrowblocks(transa);
  ptrdiff_t n = a.ncolblocks(transa);
  assert(b.nrowblocks(transb) == m);
  assert(b.ncolblocks(transb) == n);
  blocked_matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j)
    for (ptrdiff_t i = 0; i < m; ++i)
      r.set(i, j, fmap2(subm_action(), a(i, j, transa), b(i, j, transb), transa,
                        transb));
  return r;
}

// Level 2

// gerb

// Level 3

blocked_matrix_t gemmb(const blocked_matrix_t &a, const blocked_matrix_t &b,
                       bool transa, bool transb, double alpha) {
  ptrdiff_t m = a.nrowblocks(transa);
  ptrdiff_t l = a.ncolblocks(transa);
  ptrdiff_t n = b.ncolblocks(transb);
  assert(b.nrowblocks(transb) == l);
  blocked_matrix_t r(m, n);
  for (ptrdiff_t j = 0; j < n; ++j) {
    for (ptrdiff_t i = 0; i < m; ++i) {
      vector<client<matrix_t> > ress(l);
      for (ptrdiff_t k = 0; k < l; ++k) {
        ress[k] = fmap2(gemm_action(), a(i, k, transa), b(k, j, transb), transa,
                        transb, alpha);
      }
      client<matrix_t> res =
          foldl1([transa, transb](const client<matrix_t> &a,
                                  const client<matrix_t> &b) {
                   return fmap2(addm_action(), a, b, transa, transb);
                 },
                 ress);
      r.set(i, j, res);
    }
  }
  return r;
}
}
