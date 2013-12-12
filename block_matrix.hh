#ifndef BLOCK_MATRIX_HH
#define BLOCK_MATRIX_HH

#include "matrix.hh"

#include "rpc.hh"

#include <boost/shared_ptr.hpp>

#include <cassert>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>
#include <vector>



struct block_vector_t;
struct block_matrix_t;



struct structure_t {
  typedef boost::shared_ptr<const structure_t> const_ptr;
  
  const std::ptrdiff_t N, B;
  const std::vector<std::ptrdiff_t> begin;
  const std::vector<int> locs;
  bool invariant() const;
  structure_t(std::ptrdiff_t N,
              std::ptrdiff_t B,
              const std::ptrdiff_t* begin,
              const int* locs):
    N(N), B(B), begin(begin, begin+B+1), locs(locs, locs+B)
  {
    assert(invariant());
  }
  operator std::string() const { return mkstr(*this); }
  bool operator==(const structure_t& str) const { return this == &str; }
  std::ptrdiff_t size(std::ptrdiff_t b) const
  {
    assert(b>=0 && b<B);
    return begin[b+1] - begin[b];
  }
  std::ptrdiff_t find(std::ptrdiff_t i) const;
};

std::ostream& operator<<(std::ostream& os, const structure_t& str);



struct block_vector_t {
  typedef boost::shared_ptr<const block_vector_t> const_ptr;
  typedef boost::shared_ptr<block_vector_t> ptr;
  
  structure_t::const_ptr str;
  std::vector<unsigned char> has_block_; // vector<bool> is not thread-safe
  std::vector<rpc::client<vector_t>> blocks;
  block_vector_t(const structure_t::const_ptr& str);
  struct block_t {
    std::ptrdiff_t i;
    vector_t x;
    block_t(std::ptrdiff_t i, const vector_t& x): i(i), x(x) {}
  };
  template<typename T>
  explicit block_vector_t(const structure_t::const_ptr& str,
                          std::ptrdiff_t nblocks, const T& blocks_);
  operator std::string() const { return mkstr(*this); }
  bool has_block(std::ptrdiff_t b) const
  {
    assert(b>=0 && b<str->B);
    return has_block_[b];
  }
  void make_block(std::ptrdiff_t b);
  void remove_block(std::ptrdiff_t b);
  const rpc::client<vector_t>& block(std::ptrdiff_t b) const
  {
    assert(b>=0 && b<str->B);
    assert(has_block(b));
    return blocks[b];
  }
  void set_block(std::ptrdiff_t b, const rpc::client<vector_t>& x)
  {
    assert(b>=0 && b<str->B);
    // TODO: check that x is not NULL
    has_block_[b] = true;
    blocks[b] = x;
  }
  bool has_elt(std::ptrdiff_t i) const
  {
    assert(i>=0 && i<str->N);
    auto b = str->find(i);
    return has_block(b);
  }
  double operator()(std::ptrdiff_t i) const
  {
    assert(i>=0 && i<str->N);
    auto b = str->find(i);
    static const double zero = 0.0;
    if (!has_block(b)) return zero;
    return rpc::sync(block(b), vector_t::get_elt_action(),
                     i - str->begin[b]);
  }
  void set_elt(std::ptrdiff_t i, double x)
  {
    assert(i>=0 && i<str->N);
    auto b = str->find(i);
    assert(has_block(b));
    rpc::sync(block(b), vector_t::set_elt_action(), i - str->begin[b], x);
  }
  
  // Level 1
  auto faxpy(double alpha, const const_ptr& x) const -> ptr;
  auto fcopy() const -> ptr;
  auto fnrm2() const -> std::shared_future<double>;
  auto fscal(double alpha) const -> ptr;
  auto fset(double alpha) const -> ptr;
};

std::ostream& operator<<(std::ostream& os, const block_vector_t& x);



struct block_matrix_t {
  typedef boost::shared_ptr<const block_matrix_t> const_ptr;
  typedef boost::shared_ptr<block_matrix_t> ptr;
  
  structure_t::const_ptr istr, jstr; // interpretation: row, column
  std::vector<unsigned char> has_block_; // vector<bool> is not thread-safe
  std::vector<rpc::client<matrix_t>> blocks;
  block_matrix_t(const structure_t::const_ptr& istr,
                 const structure_t::const_ptr& jstr);
  struct block_t {
    std::ptrdiff_t i, j;
    matrix_t a;
    block_t(std::ptrdiff_t i, std::ptrdiff_t j, const matrix_t& a):
      i(i), j(j), a(a)
    {
    }
  };
  template<typename T>
  explicit block_matrix_t(const structure_t::const_ptr& istr,
                          const structure_t::const_ptr& jstr,
                          std::ptrdiff_t nblocks,
                          const T& blocks_);
  operator std::string() const { return mkstr(*this); }
  bool has_block(std::ptrdiff_t ib, std::ptrdiff_t jb) const
  {
    assert(ib>=0 && ib<istr->B && jb>=0 && jb<jstr->B);
    return has_block_[ib+istr->B*jb];
  }
  void make_block(std::ptrdiff_t ib, std::ptrdiff_t jb);
  void remove_block(std::ptrdiff_t ib, std::ptrdiff_t jb);
  const rpc::client<matrix_t>& block(std::ptrdiff_t ib, std::ptrdiff_t jb) const
  {
    assert(ib>=0 && ib<istr->B && jb>=0 && jb<jstr->B);
    assert(has_block(ib,jb));
    return blocks[ib+istr->B*jb];
  }
  void set_block(std::ptrdiff_t ib, std::ptrdiff_t jb,
                 const rpc::client<matrix_t>& a)
  {
    assert(ib>=0 && ib<istr->B && jb>=0 && jb<jstr->B);
    // TODO: check that a is not NULL
    auto b = ib+istr->B*jb;
    has_block_[b] = true;
    blocks[b] = a;
  }
  bool has_elt(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    assert(i>=0 && i<istr->N && j>=0 && j<jstr->N);
    auto ib = istr->find(i);
    auto jb = jstr->find(j);
    return has_block(ib,jb);
  }
  double operator()(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    assert(i>=0 && i<istr->N && j>=0 && j<jstr->N);
    auto ib = istr->find(i);
    auto jb = jstr->find(j);
    static const double zero = 0.0;
    if (!has_block(ib,jb)) return zero;
    return rpc::sync(block(ib,jb), matrix_t::get_elt_action(),
                     i - istr->begin[ib], j - jstr->begin[jb]);
  }
  void set_elt(std::ptrdiff_t i, std::ptrdiff_t j, double x)
  {
    assert(i>=0 && i<istr->N && j>=0 && j<jstr->N);
    auto ib = istr->find(i);
    auto jb = jstr->find(j);
    assert(has_block(ib,jb));
    rpc::sync(block(ib,jb), matrix_t::set_elt_action(),
              i - istr->begin[ib], j - jstr->begin[jb], x);
  }
  
  // Level 2
  auto faxpy(bool transa, bool transb0,
             double alpha, const const_ptr& a) const -> ptr;
  auto fcopy(bool trans) const -> ptr;
  auto fgemv(bool trans,
             double alpha, const block_vector_t::const_ptr& x,
             double beta, const block_vector_t::const_ptr& y0) const ->
    block_vector_t::ptr;
  auto fnrm2() const -> std::shared_future<double>;
  auto fscal(bool trans, double alpha) const -> ptr;
  auto fset(bool trans, double alpha) const -> ptr;
  
  // Level 3
  auto fgemm(bool transa, bool transb, bool transc0,
             double alpha, const const_ptr& a, const const_ptr& b,
             double beta) const -> ptr;
};

std::ostream& operator<<(std::ostream& os, const block_matrix_t& a);



////////////////////////////////////////////////////////////////////////////////

template<typename T>
block_vector_t::block_vector_t(const structure_t::const_ptr& str,
                               std::ptrdiff_t nblocks, const T& blocks_):
  str(str), has_block_(str->B, false), blocks(str->B)
{
  for (const auto& blk: blocks_) {
    auto b = str->find(blk.i);
    assert(str->begin[b] == blk.i);
    assert(!has_block(b));
    has_block_[b] = true;
    assert(blk.x.N == str->size(b));
    blocks[b] = rpc::make_remote_client<vector_t>(str->locs[b], blk.x);
  }
}

template<typename T>
block_matrix_t::block_matrix_t(const structure_t::const_ptr& istr,
                               const structure_t::const_ptr& jstr,
                               std::ptrdiff_t nblocks,
                               const T& blocks_):
  istr(istr), jstr(jstr),
  has_block_(istr->B*jstr->B, false), blocks(istr->B*jstr->B)
{
  for (const auto& blk: blocks_) {
    auto ib = istr->find(blk.i);
    auto jb = jstr->find(blk.j);
    assert(istr->begin[ib] == blk.i);
    assert(jstr->begin[jb] == blk.j);
    auto b = ib+istr->B*jb;
    assert(!has_block_[b]);
    has_block_[b] = true;
    assert(blk.a.NI == istr->size(ib));
    assert(blk.a.NJ == jstr->size(jb));
    blocks[b] = rpc::make_remote_client<matrix_t>(istr->locs[ib], blk.a);
  }
}

#endif // #ifndef BLOCK_MATRIX_HH
