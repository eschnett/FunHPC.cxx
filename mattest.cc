#include "rpc.hh"

#include "algorithms.hh"
#include "block_matrix.hh"
#include "matrix.hh"

#include <boost/utility/identity_type.hpp>

#include <cstdlib>
#include <iostream>



void test_dense()
{
  const int here = rpc::server->rank();
  std::cout << "test_dense: running on process " << here << std::endl;
  
  const double alpha = 2.0, beta = 3.0;
  std::cout << "alpha=" << alpha << ", beta=" << beta << std::endl;
  
  const ptrdiff_t NI=4, NJ=3, NK=2;
  std::cout << "NI=" << NI << ", NJ=" << NJ << ", NK=" << NK << std::endl;
  
  vector_t x(NJ);
  for (ptrdiff_t j=0; j<NJ; ++j) x(j) = j + 1;
  std::cout << "x=" << x << std::endl;
  vector_t y(NI);
  for (ptrdiff_t i=0; i<NI; ++i) y(i) = i + 2;
  std::cout << "y=" << y << std::endl;
  vector_t z(NI);
  for (ptrdiff_t i=0; i<NI; ++i) z(i) = i + 3;
  std::cout << "z=" << z << std::endl;
  
  matrix_t a(NI,NJ);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    for (ptrdiff_t j=0; j<NJ; ++j)
      a(i,j) = n++ + 4;
  std::cout << "a=" << a << std::endl;
  matrix_t b(NI,NK);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    for (ptrdiff_t k=0; k<NK; ++k)
      b(i,k) = n++ + 5;
  std::cout << "b=" << b << std::endl;
  matrix_t c(NK,NJ);
  for (ptrdiff_t k=0, n=0; k<NK; ++k)
    for (ptrdiff_t j=0; j<NJ; ++j)
      c(k,j) = n++ + 6;
  std::cout << "c=" << c << std::endl;
  
  const double nrm2_result = sqrt(14.0);
  const vector_t axpy_result(NI, (double[NI]){7,10,13,16});
  const vector_t gemv_result(NI, (double[NI]){70,109,148,187});
  const matrix_t gemm_result
    (NI,NJ, (double[NI][NJ]){
      {180,205,230},
      {249,282,315},
      {318,359,400},
      {387,436,485},
    });
  
  std::cout << std::endl;
  
  

  double total_error = 0.0;
  
  double nn;
  vector_t yy(NI), zz(NI);
  matrix_t aa(NI,NJ);
  
  nn = nrm2(x);
  std::cout << "nrm2: |x| = " << nn << std::endl;
  nn -= nrm2_result;
  total_error += fabs(nn);
  std::cout << "   (error = " << fabs(nn) << ")" << std::endl;
  
  copy(z, zz);
  axpy(alpha, y, zz);
  std::cout << "axpy: alpha y + z = " << zz << std::endl;
  axpy(-1.0, axpy_result, zz);
  total_error += nrm2(zz);
  std::cout << "   (error = " << nrm2(zz) << ")" << std::endl;
  
  copy(y, yy);
  gemv(false, alpha, a, x, beta, yy);
  std::cout << "gemv: alpha a x + beta y = " << yy << std::endl;
  axpy(-1.0, gemv_result, yy);
  total_error += nrm2(yy);
  std::cout << "   (error = " << nrm2(yy) << ")" << std::endl;
  
  copy(false, a, aa);
  gemm(false, false, alpha, b, c, beta, aa);
  std::cout << "gemm: alpha b c + beta a = " << aa << std::endl;
  axpy(false, -1.0, gemm_result, aa);
  total_error += nrm2(aa);
  std::cout << "   (error = " << nrm2(aa) << ")" << std::endl;
  
  
  
  struct null_deleter { void operator()(const void*) {} };
  const vector_t::const_ptr xp(&x, null_deleter());
  const vector_t::const_ptr yp(&y, null_deleter());
  const vector_t::const_ptr zp(&z, null_deleter());
  const matrix_t::const_ptr ap(&a, null_deleter());
  const matrix_t::const_ptr bp(&b, null_deleter());
  const matrix_t::const_ptr cp(&c, null_deleter());
  
  vector_t::ptr yyp, zzp;
  matrix_t::ptr aap;
  
  nn = *xp->fnrm2();
  std::cout << "fnrm2: |x| = " << nn << std::endl;
  nn -= nrm2_result;
  total_error += fabs(nn);
  std::cout << "   (error = " << fabs(nn) << ")" << std::endl;
  
  zzp = zp->faxpy(alpha, yp);
  std::cout << "faxpy: alpha y + z = " << *zzp << std::endl;
  const vector_t::const_ptr axpy_resultp(&axpy_result, null_deleter());
  zzp = zzp->faxpy(-1.0, axpy_resultp);
  total_error += *zzp->fnrm2();
  std::cout << "   (error = " << *zzp->fnrm2() << ")" << std::endl;
  
  yyp = ap->fgemv(false, alpha, xp, beta, yp);
  std::cout << "fgemv: alpha a x + beta y = " << *yyp << std::endl;
  const vector_t::const_ptr gemv_resultp(&gemv_result, null_deleter());
  yyp = yyp->faxpy(-1.0, gemv_resultp);
  total_error += *yyp->fnrm2();
  std::cout << "   (error = " << *yyp->fnrm2() << ")" << std::endl;
  
  aap = bp->fgemm(false, false, false, alpha, cp, beta, ap);
  std::cout << "fgemm: alpha b c + beta a = " << *aap << std::endl;
  const matrix_t::const_ptr gemm_resultp(&gemm_result, null_deleter());
  aap = aap->faxpy(false, false, -1.0, gemm_resultp);
  total_error += *aap->fnrm2();
  std::cout << "   (error = " << *aap->fnrm2() << ")" << std::endl;
  
  
  
  std::cout << std::endl;
  std::cout << std::flush;
  
  if (total_error > 1.0e-12) {
    std::abort();
  }
}



void test_block()
{
  int nlocs = rpc::server->size();
  std::vector<int> locs(nlocs);
  for (int loc=0; loc<nlocs; ++loc) locs[loc] = loc;
  int here = rpc::server->rank();
  std::cout << "test_block: running on process " << here << std::endl;
  
  const double alpha = 2.0, beta = 3.0;
  std::cout << "alpha=" << alpha << ", beta=" << beta << std::endl;
  
  const std::ptrdiff_t NI=10, NJ=6, NK=6;
  std::cout << "NI=" << NI << ", NJ=" << NJ << ", NK=" << NK << std::endl;
  
  const std::ptrdiff_t BI = 6;
  const std::ptrdiff_t ibegin[BI+1] = {0, 1, 2, 4, 6, 9, 10};
  int ilocs[BI];
  for (std::ptrdiff_t i=0; i<BI; ++i) ilocs[i] = locs[i % nlocs];
  auto istr = boost::make_shared<structure_t>(NI, BI, ibegin, ilocs);
  std::cout << "istr=" << *istr << std::endl;
  
  const std::ptrdiff_t BJ = 4;
  const std::ptrdiff_t jbegin[BJ+1] = {0, 2, 4, 5, 6};
  int jlocs[BJ];
  for (std::ptrdiff_t j=0; j<BJ; ++j) jlocs[j] = locs[(j+1) % nlocs];
  auto jstr = boost::make_shared<structure_t>(NJ, BJ, jbegin, jlocs);
  std::cout << "jstr=" << *jstr << std::endl;
  
  const std::ptrdiff_t BK = 3;
  const std::ptrdiff_t kbegin[BK+1] = {0, 1, 3, 6};
  int klocs[BK];
  for (std::ptrdiff_t k=0; k<BK; ++k) klocs[k] = locs[(k+2) % nlocs];
  auto kstr = boost::make_shared<structure_t>(NK, BK, kbegin, klocs);
  std::cout << "kstr=" << *kstr << std::endl;
  
  block_vector_t x(jstr);
  for (ptrdiff_t jb=0; jb<BJ; ++jb)
    if (jb % 2 == 0)
      x.make_block(jb);
  for (ptrdiff_t j=0, n=0; j<NJ; ++j)
    if (x.has_elt(j))
      x.set_elt(j, n++ + 1);
  std::cout << "x=" << x << std::endl;
  
  block_vector_t y(istr);
  for (ptrdiff_t ib=0; ib<BI; ++ib)
    if (ib % 2 == 1)
      y.make_block(ib);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    if (y.has_elt(i))
      y.set_elt(i, n++ + 2);
  std::cout << "y=" << y << std::endl;
  
  block_vector_t z(istr);
  for (ptrdiff_t ib=0; ib<BI; ++ib)
    if (ib % 2 == 1)
      z.make_block(ib);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    if (z.has_elt(i))
      z.set_elt(i, n++ + 3);
  std::cout << "z=" << z << std::endl;
  
  block_matrix_t a(istr,jstr);
  for (ptrdiff_t ib=0; ib<BI; ++ib)
    if (ib % 2 == 1)
      for (ptrdiff_t jb=0; jb<BJ; ++jb)
        if (jb % 2 == 0)
          a.make_block(ib,jb);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    for (ptrdiff_t j=0; j<NJ; ++j)
      if (a.has_elt(i,j))
        a.set_elt(i,j, n++ + 4);
  std::cout << "a=" << a << std::endl;
  
  block_matrix_t b(istr,kstr);
  for (ptrdiff_t ib=0; ib<BI; ++ib)
    if (ib % 2 == 1)
      for (ptrdiff_t kb=0; kb<BK; ++kb)
        if (kb % 2 == 1)
          b.make_block(ib,kb);
  for (ptrdiff_t i=0, n=0; i<NI; ++i)
    for (ptrdiff_t k=0; k<NK; ++k)
      if (b.has_elt(i,k))
        b.set_elt(i,k, n++ + 5);
  std::cout << "b=" << b << std::endl;
  
  block_matrix_t c(kstr,jstr);
  for (ptrdiff_t kb=0; kb<BK; ++kb)
    if (kb % 2 == 1)
      for (ptrdiff_t jb=0; jb<BJ; ++jb)
        if (jb % 2 == 0)
          c.make_block(kb,jb);
  for (ptrdiff_t k=0, n=0; k<NK; ++k)
    for (ptrdiff_t j=0; j<NJ; ++j)
      if (c.has_elt(k,j))
        c.set_elt(k,j, n++ + 6);
  std::cout << "c=" << c << std::endl;
  
  const double nrm2_x_result = sqrt(14.0);
  const double nrm2_a_result = sqrt(1226.0);

  typedef block_vector_t::block_t B;
  typedef vector_t V;
  const block_vector_t axpy_result
    (istr, {
      B(1,V(1,(double[1]){7})),
      B(4,V(2,(double[2]){10,13})),
      B(9,V(1,(double[1]){16}))
      });
  const block_vector_t gemv_result
    (istr, {
      B(1,V(1,(double[1]){70})),
      B(4,V(2,(double[2]){109,148})),
      B(9,V(1,(double[1]){187})),
      });
  
  typedef block_matrix_t::block_t A;
  typedef matrix_t M;
  const block_matrix_t gemm_result
    (istr,jstr, {
      A(1,0, M(1,2,(double[1][2]){{180,205}})),
      A(1,4, M(1,1,(double[1][1]){{230}})),
      A(4,0, M(2,2,(double[2][2]){{249,282}, {318,359}})),
      A(4,4, M(2,1,(double[2][1]){{315}, {400}})),
      A(9,0, M(1,2,(double[1][2]){{387,436}})),
      A(9,4, M(1,1,(double[1][1]){{485}})),
      });
  
  std::cout << std::endl;
  
  
  
  double total_error = 0.0;
  
  struct null_deleter { void operator()(const void*) {} };
  const block_vector_t::const_ptr xp(&x, null_deleter());
  const block_vector_t::const_ptr yp(&y, null_deleter());
  const block_vector_t::const_ptr zp(&z, null_deleter());
  const block_matrix_t::const_ptr ap(&a, null_deleter());
  const block_matrix_t::const_ptr bp(&b, null_deleter());
  const block_matrix_t::const_ptr cp(&c, null_deleter());
  
  double nn;
  block_vector_t::ptr yyp, zzp;
  block_matrix_t::ptr aap;
  
  nn = *xp->fnrm2().make_local();
  std::cout << "fnrm2: |x| = " << nn << std::endl;
  nn -= nrm2_x_result;
  const double fnrm2_x_error = fabs(nn);
  std::cout << "   (error = " << fnrm2_x_error << ")" << std::endl;
  total_error += fnrm2_x_error;
  
  zzp = zp->faxpy(alpha, yp);
  std::cout << "faxpy: alpha y + z = " << *zzp << std::endl;
  const block_vector_t::const_ptr axpy_resultp(&axpy_result, null_deleter());
  zzp = zzp->faxpy(-1.0, axpy_resultp);
  const double faxpy_error = *zzp->fnrm2().make_local();
  std::cout << "   (error = " << faxpy_error << ")" << std::endl;
  total_error += faxpy_error;
  
  yyp = ap->fgemv(false, alpha, xp, beta, yp);
  std::cout << "fgemv: alpha a x + beta y = " << *yyp << std::endl;
  const block_vector_t::const_ptr gemv_resultp(&gemv_result, null_deleter());
  yyp = yyp->faxpy(-1.0, gemv_resultp);
  const double fgemv_error = *zzp->fnrm2().make_local();
  std::cout << "   (error = " << fgemv_error << ")" << std::endl;
  total_error += fgemv_error;
  
  nn = *ap->fnrm2().make_local();
  std::cout << "fnrm2: |a| = " << nn << std::endl;
  nn -= nrm2_a_result;
  const double fnrm2_a_error = fabs(nn);
  std::cout << "   (error = " << fnrm2_a_error << ")" << std::endl;
  total_error += fnrm2_a_error;
  
  aap = bp->fgemm(false, false, false, alpha, cp, beta, ap);
  std::cout << "fgemm: alpha b c + beta a = " << *aap << std::endl;
  const block_matrix_t::const_ptr gemm_resultp(&gemm_result, null_deleter());
  aap = aap->faxpy(false, false, -1.0, gemm_resultp);
  const double fgemm_error = *zzp->fnrm2().make_local();
  std::cout << "   (error = " << fgemm_error << ")" << std::endl;
  total_error += fgemm_error;
  
  
  
  std::cout << std::endl;
  std::cout << std::flush;
  
  if (total_error > 1.0e-12) {
    std::abort();
  }
}



int rpc_main(int argc, char** argv)
{
  test_dense();
  test_block();
  return 0;
}
