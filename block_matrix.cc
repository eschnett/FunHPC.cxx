#include "block_matrix.hh"

#include "algorithms.hh"
#include "matrix.hh"

#include "rpc.hh"

#include <boost/utility/identity_type.hpp>

#include <cmath>



bool structure_t::invariant() const
{
  if (N < 0) return false;
  if (B < 0) return false;
  if (begin[0] != 0) return false;
  for (std::ptrdiff_t b=0; b<B; ++b) {
    if (begin[b+1] <= begin[b]) return false;
  }
  if (begin[B] != N) return false;
  return true;
}

std::ptrdiff_t structure_t::find(std::ptrdiff_t i) const
{
  RPC_ASSERT(i>=0 && i<N);
  std::ptrdiff_t b0 = 0, b1 = B;
  auto loopinv = [&]() { return b0>=0 && b1<=B && b0<b1; };
  auto loopvar = [&]() { return b1 - b0; };
  RPC_ASSERT(loopinv());
  std::ptrdiff_t old_loopvar = loopvar();
  while (b0+1 < b1 && i>=begin[b0] && i<begin[b1]) {
    std::ptrdiff_t b = (b0 + b1)/2;
    RPC_ASSERT(b>b0 && b<b1);
    if (i >= begin[b])
      b0 = b;
    else
      b1 = b;
    RPC_ASSERT(loopinv());
    auto next_loopvar = loopvar();
    RPC_ASSERT(next_loopvar >= 0 && next_loopvar < old_loopvar);
    old_loopvar = next_loopvar;
  }
  RPC_ASSERT(loopinv());
  return b0;
}

std::ostream& operator<<(std::ostream& os, const structure_t& str)
{
  os << "{";
  for (std::ptrdiff_t b=0; b<str.B; ++b) {
    if (b != 0) os << ",";
    os << str.begin[b];
  }
  os << "}";
  return os;
}



block_vector_t::block_vector_t(const structure_t::const_ptr& str):
  str(str), has_block_(str->B, false), blocks(str->B)
{
}

void block_vector_t::make_block(std::ptrdiff_t b)
{
  RPC_ASSERT(b>=0 && b<str->B);
  RPC_ASSERT(!has_block(b));
  has_block_[b] = true;
  blocks[b] = rpc::make_remote_client<vector_t>(str->locs[b], str->size(b));
}
// TODO: avoid this
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<vector_t, std::ptrdiff_t>))::evaluate);
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<vector_t, std::ptrdiff_t>))::finish);

void block_vector_t::remove_block(std::ptrdiff_t b)
{
  RPC_ASSERT(b>=0 && b<str->B);
  RPC_ASSERT(has_block(b));
  has_block_[b] = false;
  blocks[b] = rpc::client<vector_t>();
}

std::ostream& operator<<(std::ostream& os, const block_vector_t& x)
{
  os << "{";
  for (std::ptrdiff_t b=0; b<x.str->B; ++b) {
    if (b != 0) os << ",";
    os << x.str->begin[b];
    if (x.has_block(b)) {
      os << ":" << *x.block(b).make_local();
    }
  }
  os << "}";
  return os;
}

// Level 1

auto block_vector_t::faxpy(double alpha, const const_ptr& x) const -> ptr
{
  if (alpha == 0.0) return fcopy();
  RPC_ASSERT(str == x->str);
  auto y = boost::make_shared<block_vector_t>(str);
  for (std::ptrdiff_t ib=0; ib<y->str->B; ++ib) {
    if (!x->has_block(ib)) {
      if (has_block(ib)) {
        // y->set_block(ib, afcopy(block(ib)));
        y->set_block(ib, block(ib));
      }
    } else {
      if (!has_block(ib)) {
        y->set_block(ib, afscal(alpha, x->block(ib)));
      } else {
        y->set_block(ib, afaxpy(alpha, x->block(ib), block(ib)));
      }
    }
  }
  return y;
}

auto block_vector_t::fcopy() const -> ptr
{
  auto y = boost::make_shared<block_vector_t>(str);
  for (std::ptrdiff_t ib=0; ib<y->str->B; ++ib) {
    if (has_block(ib)) {
      // y->set_block(ib, afcopy(block(ib)));
      y->set_block(ib, block(ib));
    }
  }
  return y;
}

scalar_t::client fnrm2_init()
{
  return rpc::make_client<scalar_t>(nrm2_init());
}
RPC_ACTION(fnrm2_init);
scalar_t::client fnrm2_process(scalar_t::client xi)
{
  RPC_ASSERT(xi.is_local());
  return rpc::make_client<scalar_t>(nrm2_process(*xi));
}
RPC_ACTION(fnrm2_process);
scalar_t::client fnrm2_combine(scalar_t::client val0, scalar_t::client val1)
{
  RPC_ASSERT(val0.is_local());
  return rpc::make_client<scalar_t>(nrm2_combine(*val0, *val1.make_local()));
}
RPC_ACTION(fnrm2_combine);
scalar_t::client fnrm2_finalize(scalar_t::client val)
{
  RPC_ASSERT(val.is_local());
  return rpc::make_client<scalar_t>(nrm2_finalize(*val));
}
RPC_ACTION(fnrm2_finalize);

auto block_vector_t::fnrm2() const -> scalar_t::client
{
  // TODO: use map_reduce instead of creating an intermediate vector.
  // extend map_reduce to allow skipping (or producing multiple)
  // elements.
  auto fs = rpc::make_client<std::vector<scalar_t::client> >();
  for (std::ptrdiff_t ib=0; ib<str->B; ++ib) {
    if (has_block(ib)) {
      fs->push_back(afnrm2(block(ib)));
    }
  }
  return rpc::async(rpc::map_reduce(fnrm2_process_action(),
                                    fnrm2_combine_action(),
                                    fnrm2_init_action(),
                                    fs),
                    fnrm2_finalize_action());
}

auto block_vector_t::fscal(double alpha) const -> ptr
{
  auto x = boost::make_shared<block_vector_t>(str);
  if (alpha != 0.0) {
    for (std::ptrdiff_t ib=0; ib<x->str->B; ++ib) {
      if (has_block(ib)) {
        x->set_block(ib, afscal(alpha, block(ib)));
      }
    }
  }
  return x;
}

auto block_vector_t::fset(double alpha) const -> ptr
{
  auto x = boost::make_shared<block_vector_t>(str);
  if (alpha != 0.0) {
    for (std::ptrdiff_t ib=0; ib<x->str->B; ++ib) {
      if (!has_block(ib)) {
        const auto tmp =
          rpc::make_remote_client<vector_t>(str->locs[ib], str->size(ib));
        x->set_block(ib, afset(alpha, tmp));
      } else {
        x->set_block(ib, afset(alpha, block(ib)));
      }
    }
  }
  return x;
}

BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<vector_t, vector_t>))::evaluate);
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<vector_t, vector_t>))::finish);



block_matrix_t::block_matrix_t(const structure_t::const_ptr& istr,
                               const structure_t::const_ptr& jstr):
  istr(istr), jstr(jstr),
  has_block_(istr->B*jstr->B, false), blocks(istr->B*jstr->B)
{
}

void block_matrix_t::make_block(std::ptrdiff_t ib, std::ptrdiff_t jb)
{
  RPC_ASSERT(ib>=0 && ib<istr->B && jb>=0 && jb<jstr->B);
  RPC_ASSERT(!has_block(ib,jb));
  auto b = ib+istr->B*jb;
  has_block_[b] = true;
  blocks[b] = rpc::make_remote_client<matrix_t>(istr->locs[ib],
                                                istr->size(ib), jstr->size(jb));
}
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<matrix_t, std::ptrdiff_t, std::ptrdiff_t>))::evaluate);
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<matrix_t, std::ptrdiff_t, std::ptrdiff_t>))::finish);

void block_matrix_t::remove_block(std::ptrdiff_t ib, std::ptrdiff_t jb)
{
  RPC_ASSERT(ib>=0 && ib<istr->B && jb>=0 && jb<jstr->B);
  RPC_ASSERT(has_block(ib,jb));
  auto b = ib+istr->B*jb;
  has_block_[b] = false;
  blocks[b] = rpc::client<matrix_t>();
}

std::ostream& operator<<(std::ostream& os, const block_matrix_t& a)
{
  os << "{";
  for (std::ptrdiff_t ib=0; ib<a.istr->B; ++ib) {
    if (ib != 0) os << ",";
    os << "{";
    for (std::ptrdiff_t jb=0; jb<a.jstr->B; ++jb) {
      if (jb != 0) os << ",";
      os << "(" << a.istr->begin[ib] << "," << a.jstr->begin[jb] << ")";
      if (a.has_block(ib,jb)) {
        os << ":" << *a.block(ib,jb).make_local();
      }
    }
    os << "}";
  }
  os << "}";
  return os;
}

// Level 2

auto block_matrix_t::faxpy(bool transa, bool transb0,
                           double alpha, const const_ptr& a) const -> ptr
{
  if (alpha == 0.0) return fcopy(transb0);
  const auto& istra = !transa ? a->istr : a->jstr;
  const auto& jstra = !transa ? a->jstr : a->istr;
  const auto& istrb0 = !transb0 ? istr : jstr;
  const auto& jstrb0 = !transb0 ? jstr : istr;
  RPC_ASSERT(istrb0 == istra);
  RPC_ASSERT(jstrb0 == jstra);
  auto b = boost::make_shared<block_matrix_t>(istrb0, jstrb0);
  for (std::ptrdiff_t jb=0; jb<b->jstr->B; ++jb) {
    for (std::ptrdiff_t ib=0; ib<b->istr->B; ++ib) {
      auto iba = !transa ? ib : jb;
      auto jba = !transa ? jb : ib;
      auto ibb0 = !transb0 ? ib : jb;
      auto jbb0 = !transb0 ? jb : ib;
      if (!a->has_block(iba,jba)) {
        if (has_block(ibb0,jbb0)) {
          // b->set_block(ib,jb, afcopy(transb0, block(ibb0,jbb0)));
          if (transb0) {
            b->set_block(ib,jb, afcopy(transb0, block(ibb0,jbb0)));
          } else {
            b->set_block(ib,jb, block(ibb0,jbb0));
          }
        }
      } else {
        if (!has_block(ibb0,jbb0)) {
          b->set_block(ib,jb, afscal(transa, alpha, a->block(iba,jba)));
        } else {
          b->set_block(ib,jb, afaxpy(transa, transb0,
                                     alpha, a->block(iba,jba),
                                     block(ibb0,jbb0)));
        }
      }
    }
  }
  return b;
}

auto block_matrix_t::fcopy(bool trans) const -> ptr
{
  const auto& istrb = !trans ? istr : jstr;
  const auto& jstrb = !trans ? jstr : istr;
  auto b = boost::make_shared<block_matrix_t>(istrb, jstrb);
  for (std::ptrdiff_t jb=0; jb<b->jstr->B; ++jb) {
    for (std::ptrdiff_t ib=0; ib<b->istr->B; ++ib) {
      auto iba = !trans ? ib : jb;
      auto jba = !trans ? jb : ib;
      if (has_block(iba,jba)) {
        // b->set_block(ib,jb, afcopy(trans, block(iba,jba)));
        if (trans) {
          b->set_block(ib,jb, afcopy(trans, block(iba,jba)));
        } else {
          b->set_block(ib,jb, block(iba,jba));
        }
      }
    }
  }
  return b;
}

auto vector_t_add(const vector_t::client& x, const vector_t::client& y) ->
  vector_t::client
{
  return afaxpy(1.0, x, y);
}
RPC_ACTION(vector_t_add);

auto block_matrix_t::fgemv(bool trans,
                           double alpha, const block_vector_t::const_ptr& x,
                           double beta, const block_vector_t::const_ptr& y0)
  const -> block_vector_t::ptr
{
  if (alpha == 0.0) return y0->fscal(beta);
  const auto& istra = !trans ? istr : jstr;
  const auto& jstra = !trans ? jstr : istr;
  RPC_ASSERT(x->str == jstra);
  RPC_ASSERT(y0->str == istra);
  auto y = boost::make_shared<block_vector_t>(y0->str);
  for (std::ptrdiff_t ib=0; ib<y->str->B; ++ib) {
    auto ytmps = rpc::make_client<std::vector<vector_t::client> >();
    if (beta != 0.0 && y0->has_block(ib)) {
      if (beta == 1.0) {
        ytmps->push_back(y0->block(ib));
      } else {
        ytmps->push_back(afscal(beta, y0->block(ib)));
      }
    }
    for (std::ptrdiff_t jb=0; jb<x->str->B; ++jb) {
      auto iba = !trans ? ib : jb;
      auto jba = !trans ? jb : ib;
      if (has_block(iba,jba) && x->has_block(jb)) {
        ytmps->push_back(afgemv(trans, alpha, block(iba,jba), x->block(jb),
                               0.0, vector_t::client()));
      }
    }
    if (!ytmps->empty()) {
      y->set_block(ib, rpc::reduce1(vector_t_add_action(),
                                    ytmps, ytmps->begin(), ytmps->end()));
      y->block(ib).wait();
    }
  }
  // std::cerr << "b5\n";
  return y;
}

auto block_matrix_t::fnrm2() const -> scalar_t::client
{
  auto fs = rpc::make_client<std::vector<scalar_t::client> >();
  // TODO: Parallelize jb loop
  for (std::ptrdiff_t jb=0; jb<jstr->B; ++jb) {
    for (std::ptrdiff_t ib=0; ib<istr->B; ++ib) {
      if (has_block(ib,jb)) {
        fs->push_back(afnrm2(block(ib,jb)));
      }
    }
  }
  return rpc::async(rpc::map_reduce(fnrm2_process_action(),
                                    fnrm2_combine_action(),
                                    fnrm2_init_action(),
                                    fs),
                    fnrm2_finalize_action());
}

auto block_matrix_t::fscal(bool trans, double alpha) const -> ptr
{
  const auto& istrb = !trans ? istr : jstr;
  const auto& jstrb = !trans ? jstr : istr;
  auto b = boost::make_shared<block_matrix_t>(istrb, jstrb);
  if (alpha != 0.0) {
    for (std::ptrdiff_t jb=0; jb<b->jstr->B; ++jb) {
      for (std::ptrdiff_t ib=0; ib<b->istr->B; ++ib) {
        auto iba = !trans ? ib : jb;
        auto jba = !trans ? jb : ib;
        if (has_block(iba,jba)) {
          b->set_block(ib,jb, afscal(trans, alpha, block(iba,jba)));
        }
      }
    }
  }
  return b;
}

auto block_matrix_t::fset(bool trans, double alpha) const -> ptr
{
  const auto& istrb = !trans ? istr : jstr;
  const auto& jstrb = !trans ? jstr : istr;
  auto b = boost::make_shared<block_matrix_t>(istrb, jstrb);
  if (alpha != 0.0) {
    for (std::ptrdiff_t jb=0; jb<b->jstr->B; ++jb) {
      for (std::ptrdiff_t ib=0; ib<b->istr->B; ++ib) {
        auto iba = !trans ? ib : jb;
        auto jba = !trans ? jb : ib;
        if (!has_block(iba,jba)) {
          // TODO: Add a wrapper for determining the location
          const auto tmp =
            rpc::make_remote_client<matrix_t>(istr->locs[iba],
                                              istr->size(iba), jstr->size(jba));
          b->set_block(ib,jb, afset(trans, alpha, tmp));
        } else {
          b->set_block(ib,jb, afset(trans, alpha, block(iba,jba)));
        }
      }
    }
  }
  return b;
}
  
// Level 3

auto matrix_t_add(const matrix_t::client& a, const matrix_t::client& b) ->
  matrix_t::client
{
  return afaxpy(false, false, 1.0, a, b);
}
RPC_ACTION(matrix_t_add);

auto block_matrix_t::fgemm(bool transa, bool transb, bool transc0,
                           double alpha, const const_ptr& b,
                           double beta, const const_ptr& c0) const -> ptr
{
  if (alpha == 0.0) return c0->fscal(transc0, beta);
  const auto& istra = !transa ? istr : jstr;
  const auto& jstra = !transa ? jstr : istr;
  const auto& istrb = !transb ? b->istr : b->jstr;
  const auto& jstrb = !transb ? b->jstr : b->istr;
  const auto& istrc0 = beta == 0.0 ? istra : !transc0 ? c0->istr : c0->jstr;
  const auto& jstrc0 = beta == 0.0 ? jstrb : !transc0 ? c0->jstr : c0->istr;
  RPC_ASSERT(istrb == jstra);
  RPC_ASSERT(istrc0 == istra);
  RPC_ASSERT(jstrc0 == jstrb);
  auto c = boost::make_shared<block_matrix_t>(istrc0, jstrc0);
  for (std::ptrdiff_t jb=0; jb<c->jstr->B; ++jb) {
    for (std::ptrdiff_t ib=0; ib<c->istr->B; ++ib) {
      auto ibc0 = !transc0 ? ib : jb;
      auto jbc0 = !transc0 ? jb : ib;
      auto ctmps = rpc::make_client<std::vector<matrix_t::client> >();
      if (beta != 0.0 && c0->has_block(ibc0,jbc0)) {
        if (beta == 1.0 && !transc0) {
          ctmps->push_back(c0->block(ibc0,jbc0));
        } else {
          ctmps->push_back(afscal(transc0, beta, c0->block(ibc0,jbc0)));
        }
      }
      for (std::ptrdiff_t kb=0; kb<jstr->B; ++kb) {
        auto iba = !transa ? ib : kb;
        auto kba = !transa ? kb : ib;
        auto kbb = !transb ? kb : jb;
        auto jbb = !transb ? jb : kb;
        if (has_block(iba,kba) && b->has_block(kbb,jbb)) {
          ctmps->push_back(afgemm(transa, transb, false,
                                  alpha, block(iba,kba), b->block(kbb,jbb),
                                  0.0, matrix_t::client()));
        }
      }
      if (!ctmps->empty()) {
        c->set_block(ib,jb, rpc::reduce1(matrix_t_add_action(),
                                         ctmps, ctmps->begin(), ctmps->end()));
      }
    }
  }
  return c;
}

BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<matrix_t, matrix_t>))::evaluate);
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::make_global_shared_action<matrix_t, matrix_t>))::finish);
