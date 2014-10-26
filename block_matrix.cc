#include "block_matrix.hh"

#include "algorithms.hh"
#include "matrix.hh"

#include "rpc.hh"

#include <cmath>
#include <iostream>

bool structure_t::invariant() const {
  if (N < 0)
    return false;
  if (B < 0)
    return false;
  if (begin[0] != 0)
    return false;
  for (std::ptrdiff_t b = 0; b < B; ++b) {
    if (begin[b + 1] <= begin[b])
      return false;
  }
  if (begin[B] != N)
    return false;
  return true;
}

std::ptrdiff_t structure_t::find_block(std::ptrdiff_t i) const {
  RPC_ASSERT(i >= 0 && i < N);
  std::ptrdiff_t b0 = 0, b1 = B;
  auto loopinv = [&]() { return b0 >= 0 && b1 <= B && b0 < b1; };
  auto loopvar = [&]() { return b1 - b0; };
  RPC_ASSERT(loopinv());
  std::ptrdiff_t old_loopvar = loopvar();
  while (b0 + 1 < b1 && i >= begin[b0] && i < begin[b1]) {
    std::ptrdiff_t b = (b0 + b1) / 2;
    RPC_ASSERT(b > b0 && b < b1);
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

std::ostream &structure_t::output(std::ostream &os) const {
  os << "{";
  for (std::ptrdiff_t b = 0; b < B; ++b) {
    if (b != 0)
      os << ",";
    os << begin[b];
  }
  os << "}";
  return os;
}

block_vector_t::block_vector_t(const structure_t::const_ptr &str)
    : str(str), has_block_(str->num_blocks(), false),
      blocks_(str->num_blocks()) {}

block_vector_t::block_vector_t(const structure_t::const_ptr &str,
                               const std::initializer_list<block_t> &blks)
    : str(str), has_block_(str->num_blocks(), false),
      blocks_(str->num_blocks()) {
  for (const auto &blk : blks) {
    auto b = str->find_block(blk.i);
    RPC_ASSERT(str->block_begin(b) == blk.i);
    RPC_ASSERT(!has_block(b));
    has_block_[b] = true;
    RPC_ASSERT(blk.x.N == str->block_size(b));
    int proc = choose_proc(b);
    blocks_[b] = rpc::make_remote_client<vector_t>(proc, blk.x);
  }
}

void block_vector_t::make_block(std::ptrdiff_t b) {
  RPC_ASSERT(!has_block(b));
  has_block_[b] = true;
  int proc = choose_proc(b);
  blocks_[b] = rpc::make_remote_client<vector_t>(proc, block_size(b));
}
// TODO: avoid this
RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE(
    (rpc::make_global_shared_action<vector_t, std::ptrdiff_t>::evaluate)));
RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE(
    (rpc::make_global_shared_action<vector_t, std::ptrdiff_t>::finish)));

void block_vector_t::remove_block(std::ptrdiff_t b) {
  RPC_ASSERT(has_block(b));
  has_block_[b] = false;
  blocks_[b] = rpc::client<vector_t>();
}

std::ostream &block_vector_t::output(std::ostream &os) const {
  os << "{";
  for (std::ptrdiff_t b = 0; b < num_blocks(); ++b) {
    if (b != 0)
      os << ",";
    os << block_begin(b);
    if (has_block(b)) {
      os << ":" << *block(b).make_local();
    }
  }
  os << "}";
  return os;
}

// Level 1

auto block_vector_t::faxpy(double alpha, const const_ptr &x) const -> ptr {
  if (alpha == 0.0)
    return fcopy();
  RPC_ASSERT(str == x->str);
  auto y = std::make_shared<block_vector_t>(str);
  for (std::ptrdiff_t ib = 0; ib < y->num_blocks(); ++ib) {
    if (!x->has_block(ib)) {
      if (has_block(ib)) {
        y->set_block(ib, block(ib));
      }
    } else {
      if (!has_block(ib)) {
        if (alpha == 1.0) {
          y->set_block(ib, x->block(ib));
        } else {
          y->set_block(ib, afscal(alpha, x->block(ib)));
        }
      } else {
        y->set_block(ib, afaxpy(alpha, x->block(ib), block(ib)));
      }
    }
  }
  return y;
}

auto block_vector_t::fcopy() const -> ptr {
  auto y = std::make_shared<block_vector_t>(str);
  for (std::ptrdiff_t ib = 0; ib < y->num_blocks(); ++ib) {
    if (has_block(ib)) {
      y->set_block(ib, block(ib));
    }
  }
  return y;
}

scalar_t::client fnrm2_init() {
  return rpc::make_client<scalar_t>(nrm2_init());
}
RPC_ACTION(fnrm2_init);
scalar_t::client fnrm2_process(scalar_t::client xi) {
  RPC_ASSERT(xi.is_local());
  return rpc::make_client<scalar_t>(nrm2_process(*xi));
}
RPC_ACTION(fnrm2_process);
scalar_t::client fnrm2_combine(scalar_t::client val0, scalar_t::client val1) {
  RPC_ASSERT(val0.is_local());
  return rpc::make_client<scalar_t>(nrm2_combine(*val0, *val1.make_local()));
}
RPC_ACTION(fnrm2_combine);
scalar_t::client fnrm2_finalize(scalar_t::client val) {
  RPC_ASSERT(val.is_local());
  return rpc::make_client<scalar_t>(nrm2_finalize(*val));
}
RPC_ACTION(fnrm2_finalize);

auto block_vector_t::fnrm2() const -> scalar_t::client {
  // TODO: use map_reduce instead of creating an intermediate vector.
  // extend map_reduce to allow skipping (or producing multiple)
  // elements.
  auto fs = rpc::make_client<std::vector<scalar_t::client> >();
  for (std::ptrdiff_t ib = 0; ib < num_blocks(); ++ib) {
    if (has_block(ib)) {
      fs->push_back(afnrm2(block(ib)));
    }
  }
  return local(fnrm2_finalize_action(),
               rpc::map_reduce(fnrm2_process_action(), fnrm2_combine_action(),
                               fnrm2_init_action(), fs));
}

auto block_vector_t::fscal(double alpha) const -> ptr {
  auto x = std::make_shared<block_vector_t>(str);
  if (alpha != 0.0) {
    for (std::ptrdiff_t ib = 0; ib < x->num_blocks(); ++ib) {
      if (has_block(ib)) {
        if (alpha == 1.0) {
          x->set_block(ib, block(ib));
        } else {
          x->set_block(ib, afscal(alpha, block(ib)));
        }
      }
    }
  }
  return x;
}

auto block_vector_t::fset(double alpha) const -> ptr {
  auto x = std::make_shared<block_vector_t>(str);
  if (alpha != 0.0) {
    for (std::ptrdiff_t ib = 0; ib < x->num_blocks(); ++ib) {
      if (!has_block(ib)) {
        int proc = choose_proc(ib);
        const auto tmp =
            rpc::make_remote_client<vector_t>(proc, block_size(ib));
        x->set_block(ib, afset(alpha, tmp));
      } else {
        x->set_block(ib, afset(alpha, block(ib)));
      }
    }
  }
  return x;
}

block_matrix_t::block_matrix_t(const structure_t::const_ptr &istr,
                               const structure_t::const_ptr &jstr)
    : strs({ { istr, jstr } }), has_block_(linear_blocks(), false),
      blocks_(linear_blocks()) {}

block_matrix_t::block_matrix_t(const structure_t::const_ptr &istr,
                               const structure_t::const_ptr &jstr,
                               const std::initializer_list<block_t> &blks)
    : strs({ { istr, jstr } }), has_block_(linear_blocks(), false),
      blocks_(linear_blocks()) {
  for (const auto &blk : blks) {
    auto ib = strs[0]->find_block(blk.i);
    auto jb = strs[1]->find_block(blk.j);
    RPC_ASSERT(strs[0]->block_begin(ib) == blk.i);
    RPC_ASSERT(strs[1]->block_begin(jb) == blk.j);
    auto b = linear_block(ib, jb);
    RPC_ASSERT(!has_block_[b]);
    has_block_[b] = true;
    RPC_ASSERT(blk.a.NI == strs[0]->block_size(ib));
    RPC_ASSERT(blk.a.NJ == strs[1]->block_size(jb));
    int proc = choose_proc(ib, jb);
    blocks_[b] = rpc::make_remote_client<matrix_t>(proc, blk.a);
  }
}

void block_matrix_t::make_block(std::ptrdiff_t ib, std::ptrdiff_t jb) {
  RPC_ASSERT(!has_block(ib, jb));
  auto b = linear_block(ib, jb);
  has_block_[b] = true;
  int proc = choose_proc(ib, jb);
  blocks_[b] = rpc::make_remote_client<matrix_t>(proc, block_size(0, ib),
                                                 block_size(1, jb));
}
RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE((rpc::make_global_shared_action<
    matrix_t, std::ptrdiff_t, std::ptrdiff_t>::evaluate)));
RPC_CLASS_EXPORT(RPC_IDENTITY_TYPE((rpc::make_global_shared_action<
    matrix_t, std::ptrdiff_t, std::ptrdiff_t>::finish)));

void block_matrix_t::remove_block(std::ptrdiff_t ib, std::ptrdiff_t jb) {
  RPC_ASSERT(has_block(ib, jb));
  auto b = linear_block(ib, jb);
  has_block_[b] = false;
  blocks_[b] = rpc::client<matrix_t>();
}

std::ostream &block_matrix_t::output(std::ostream &os) const {
  os << "{";
  for (std::ptrdiff_t ib = 0; ib < num_blocks(0); ++ib) {
    if (ib != 0)
      os << ",";
    os << "{";
    for (std::ptrdiff_t jb = 0; jb < num_blocks(1); ++jb) {
      if (jb != 0)
        os << ",";
      os << "(" << block_begin(0, ib) << "," << block_begin(1, jb) << ")";
      if (has_block(ib, jb)) {
        os << ":" << *block(ib, jb).make_local();
      }
    }
    os << "}";
  }
  os << "}";
  return os;
}

// Level 2

auto block_matrix_t::faxpy(bool transa, bool transb0, double alpha,
                           const const_ptr &a) const -> ptr {
  if (alpha == 0.0)
    return fcopy(transb0);
  auto astrs = a->strs;
  if (transa)
    std::swap(astrs[0], astrs[1]);
  auto b0strs = strs;
  if (transb0)
    std::swap(b0strs[0], b0strs[1]);
  RPC_ASSERT(b0strs == astrs);
  auto b = std::make_shared<block_matrix_t>(b0strs[0], b0strs[1]);
  for (std::ptrdiff_t jb = 0; jb < b->num_blocks(1); ++jb) {
    for (std::ptrdiff_t ib = 0; ib < b->num_blocks(0); ++ib) {
      auto iba = !transa ? ib : jb;
      auto jba = !transa ? jb : ib;
      auto ibb0 = !transb0 ? ib : jb;
      auto jbb0 = !transb0 ? jb : ib;
      if (!a->has_block(ib, jb)) {
        if (has_block(ibb0, jbb0)) {
          if (!transb0) {
            b->set_block(ib, jb, block(ibb0, jbb0));
          } else {
            b->set_block(ib, jb, afcopy(transb0, block(ibb0, jbb0)));
          }
        }
      } else {
        if (!has_block(ibb0, jbb0)) {
          b->set_block(ib, jb, afscal(transa, alpha, a->block(iba, jba)));
        } else {
          b->set_block(ib, jb, afaxpy(transa, transb0, alpha,
                                      a->block(iba, jba), block(ibb0, jbb0)));
        }
      }
    }
  }
  return b;
}

auto block_matrix_t::fcopy(bool trans) const -> ptr {
  auto a0strs = strs;
  if (trans)
    std::swap(a0strs[0], a0strs[1]);
  auto b = std::make_shared<block_matrix_t>(a0strs[0], a0strs[1]);
  for (std::ptrdiff_t jb = 0; jb < b->num_blocks(1); ++jb) {
    for (std::ptrdiff_t ib = 0; ib < b->num_blocks(0); ++ib) {
      auto iba = !trans ? ib : jb;
      auto jba = !trans ? jb : ib;
      if (has_block(iba, jba)) {
        if (!trans) {
          b->set_block(ib, jb, block(iba, jba));
        } else {
          b->set_block(ib, jb, afcopy(trans, block(iba, jba)));
        }
      }
    }
  }
  return b;
}

auto vector_t_add(const vector_t::client &x, const vector_t::client &y)
    -> vector_t::client {
  return afaxpy(1.0, x, y);
}
RPC_ACTION(vector_t_add);

auto block_matrix_t::fgemv(
    bool trans, double alpha, const block_vector_t::const_ptr &x, double beta,
    const block_vector_t::const_ptr &y0) const -> block_vector_t::ptr {
  if (alpha == 0.0)
    return y0->fscal(beta);
  auto astrs = strs;
  if (trans)
    std::swap(astrs[0], astrs[1]);
  RPC_ASSERT(x->str == astrs[1]);
  RPC_ASSERT(y0->str == astrs[0]);
  auto y = std::make_shared<block_vector_t>(y0->str);
  for (std::ptrdiff_t ib = 0; ib < y->num_blocks(); ++ib) {
    auto ytmps = rpc::make_client<std::vector<vector_t::client> >();
    if (beta != 0.0 && y0->has_block(ib)) {
      if (beta == 1.0) {
        ytmps->push_back(y0->block(ib));
      } else {
        ytmps->push_back(afscal(beta, y0->block(ib)));
      }
    }
    for (std::ptrdiff_t jb = 0; jb < x->num_blocks(); ++jb) {
      auto iba = !trans ? ib : jb;
      auto jba = !trans ? jb : ib;
      if (has_block(iba, jba) && x->has_block(jb)) {
        ytmps->push_back(afgemv(trans, alpha, block(iba, jba), x->block(jb),
                                0.0, vector_t::client()));
      }
    }
    if (!ytmps->empty()) {
      y->set_block(ib, rpc::reduce1(vector_t_add_action(), ytmps,
                                    ytmps->begin(), ytmps->end()));
      y->block(ib).wait();
    }
  }
  return y;
}

auto block_matrix_t::fnrm2() const -> scalar_t::client {
  auto fs = rpc::make_client<std::vector<scalar_t::client> >();
  // TODO: Parallelize jb loop
  for (std::ptrdiff_t jb = 0; jb < num_blocks(1); ++jb) {
    for (std::ptrdiff_t ib = 0; ib < num_blocks(0); ++ib) {
      if (has_block(ib, jb)) {
        fs->push_back(afnrm2(block(ib, jb)));
      }
    }
  }
  return local(fnrm2_finalize_action(),
               rpc::map_reduce(fnrm2_process_action(), fnrm2_combine_action(),
                               fnrm2_init_action(), fs));
}

auto block_matrix_t::fscal(bool trans, double alpha) const -> ptr {
  auto a0strs = strs;
  if (trans)
    std::swap(a0strs[0], a0strs[1]);
  auto b = std::make_shared<block_matrix_t>(a0strs[0], a0strs[1]);
  if (alpha != 0.0) {
    for (std::ptrdiff_t jb = 0; jb < b->num_blocks(1); ++jb) {
      for (std::ptrdiff_t ib = 0; ib < b->num_blocks(0); ++ib) {
        auto iba = !trans ? ib : jb;
        auto jba = !trans ? jb : ib;
        if (has_block(iba, jba)) {
          if (alpha == 1.0 && !trans) {
            b->set_block(ib, jb, block(iba, jba));
          } else {
            b->set_block(ib, jb, afscal(trans, alpha, block(iba, jba)));
          }
        }
      }
    }
  }
  return b;
}

auto block_matrix_t::fset(bool trans, double alpha) const -> ptr {
  auto a0strs = strs;
  if (trans)
    std::swap(a0strs[0], a0strs[1]);
  auto b = std::make_shared<block_matrix_t>(a0strs[0], a0strs[1]);
  if (alpha != 0.0) {
    for (std::ptrdiff_t jb = 0; jb < b->num_blocks(1); ++jb) {
      for (std::ptrdiff_t ib = 0; ib < b->num_blocks(0); ++ib) {
        auto iba = !trans ? ib : jb;
        auto jba = !trans ? jb : ib;
        if (!has_block(iba, jba)) {
          int proc = choose_proc(iba, jba);
          const auto tmp = rpc::make_remote_client<matrix_t>(
              proc, block_size(0, iba), block_size(1, jba));
          b->set_block(ib, jb, afset(trans, alpha, tmp));
        } else {
          b->set_block(ib, jb, afset(trans, alpha, block(iba, jba)));
        }
      }
    }
  }
  return b;
}

// Level 3

auto matrix_t_add(const matrix_t::client &a, const matrix_t::client &b)
    -> matrix_t::client {
  return afaxpy(false, false, 1.0, a, b);
}
RPC_ACTION(matrix_t_add);

auto block_matrix_t::fgemm(bool transa, bool transb, bool transc0, double alpha,
                           const const_ptr &b, double beta,
                           const const_ptr &c0) const -> ptr {
  if (alpha == 0.0)
    return c0->fscal(transc0, beta);
  auto astrs = strs;
  if (transa)
    std::swap(astrs[0], astrs[1]);
  auto bstrs = b->strs;
  if (transb)
    std::swap(bstrs[0], bstrs[1]);
  std::array<structure_t::const_ptr, dim> c0strs;
  if (beta == 0.0) {
    c0strs[0] = astrs[0];
    c0strs[1] = bstrs[1];
  } else {
    c0strs = c0->strs;
    if (transc0)
      std::swap(c0strs[0], c0strs[1]);
  }
  RPC_ASSERT(bstrs[0] == astrs[1]);
  RPC_ASSERT(c0strs[0] == astrs[0]);
  RPC_ASSERT(c0strs[1] == bstrs[1]);
  auto c = std::make_shared<block_matrix_t>(c0strs[0], c0strs[1]);
  std::vector<rpc::future<void> > fs;
  for (std::ptrdiff_t jb = 0; jb < c->num_blocks(1); ++jb) {
    for (std::ptrdiff_t ib = 0; ib < c->num_blocks(0); ++ib) {
      fs.push_back(rpc::async([&, ib, jb]() {
        auto ibc0 = !transc0 ? ib : jb;
        auto jbc0 = !transc0 ? jb : ib;
        auto ctmps = rpc::make_client<std::vector<matrix_t::client> >();
        if (beta != 0.0 && c0->has_block(ibc0, jbc0)) {
          if (beta == 1.0 && !transc0) {
            ctmps->push_back(c0->block(ibc0, jbc0));
          } else {
            ctmps->push_back(afscal(transc0, beta, c0->block(ibc0, jbc0)));
          }
        }
        for (std::ptrdiff_t kb = 0; kb < num_blocks(1); ++kb) {
          auto iba = !transa ? ib : kb;
          auto kba = !transa ? kb : ib;
          auto kbb = !transb ? kb : jb;
          auto jbb = !transb ? jb : kb;
          if (has_block(iba, kba) && b->has_block(kbb, jbb)) {
            ctmps->push_back(afgemm(transa, transb, false, alpha,
                                    block(iba, kba), b->block(kbb, jbb), 0.0,
                                    matrix_t::client()));
          }
        }
        if (!ctmps->empty()) {
          c->set_block(ib, jb, rpc::reduce1(matrix_t_add_action(), ctmps,
                                            ctmps->begin(), ctmps->end()));
        }
      }));
    }
  }
  for (auto &f : fs)
    f.wait(); // TODO: why wait?
  return c;
}
