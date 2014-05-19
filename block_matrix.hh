#ifndef BLOCK_MATRIX_HH
#define BLOCK_MATRIX_HH

#include "matrix.hh"

#include "rpc.hh"

#include <boost/shared_ptr.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>



class structure_t {
public:
  typedef boost::shared_ptr<const structure_t> const_ptr;
  
private:
  const std::ptrdiff_t N, B;
  const std::vector<std::ptrdiff_t> begin;
  const std::vector<int> locs;
public:
  bool invariant() const;
  structure_t(std::ptrdiff_t N,
                     std::ptrdiff_t B,
                     const std::ptrdiff_t* begin,
                     const int* locs):
    N(N), B(B), begin(begin, begin+B+1), locs(locs, locs+B)
  {
    RPC_ASSERT(invariant());
  }
  operator std::string() const { return mkstr(*this); }
  bool operator==(const structure_t& str) const { return this == &str; }
  std::ptrdiff_t size() const { return N; }
  std::ptrdiff_t num_blocks() const { return B; }
  std::ptrdiff_t block_begin(std::ptrdiff_t b) const
  {
    RPC_ASSERT(b>=0 && b<B);
    return begin[b];
  }
  std::ptrdiff_t block_size(std::ptrdiff_t b) const
  {
    RPC_ASSERT(b>=0 && b<B);
    return begin[b+1] - begin[b];
  }
  std::ptrdiff_t find_block(std::ptrdiff_t i) const;
  std::ostream& output(std::ostream& os) const;
};

inline std::ostream& operator<<(std::ostream& os, const structure_t& str)
{
  return str.output(os);
}



class block_vector_t;
class block_matrix_t;



class block_vector_t {
public:
  typedef boost::shared_ptr<const block_vector_t> const_ptr;
  typedef boost::shared_ptr<block_vector_t> ptr;
  
private:
  friend class block_matrix_t;
  structure_t::const_ptr str;
  std::vector<unsigned char> has_block_; // vector<bool> is not thread-safe
  std::vector<rpc::client<vector_t> > blocks_;
  int choose_proc(std::ptrdiff_t b) const
  {
    // Round robin
    return b % rpc::server->size();
  }
public:
  block_vector_t(const structure_t::const_ptr& str);
  struct block_t {
    std::ptrdiff_t i;
    vector_t x;
    block_t(std::ptrdiff_t i, const vector_t& x): i(i), x(x) {}
  };
  block_vector_t(const structure_t::const_ptr& str,
                 const std::initializer_list<block_t>& blocks_);
  operator std::string() const { return mkstr(*this); }
  std::ptrdiff_t num_blocks() const
  {
    return str->num_blocks();
  }
  std::ptrdiff_t block_begin(std::ptrdiff_t b) const
  {
    return str->block_begin(b);
  }
  std::ptrdiff_t block_size(std::ptrdiff_t b) const
  {
    return str->block_size(b);
  }
  bool has_block(std::ptrdiff_t b) const
  {
    RPC_ASSERT(b>=0 && b<num_blocks());
    return has_block_[b];
  }
  void make_block(std::ptrdiff_t b);
  void remove_block(std::ptrdiff_t b);
  const rpc::client<vector_t>& block(std::ptrdiff_t b) const
  {
    RPC_ASSERT(has_block(b));
    return blocks_[b];
  }
  void set_block(std::ptrdiff_t b, const rpc::client<vector_t>& x)
  {
    // TODO: check that x is not NULL
    RPC_ASSERT(b>=0 && b<num_blocks());
    has_block_[b] = true;
    blocks_[b] = x;
  }
  bool has_elt(std::ptrdiff_t i) const
  {
    auto b = str->find_block(i);
    return has_block(b);
  }
  double operator()(std::ptrdiff_t i) const
  {
    auto b = str->find_block(i);
    static const double zero = 0.0;
    if (!has_block(b)) return zero;
    return rpc::sync(vector_t::get_elt_action(),
                     block(b), i - str->block_begin(b));
  }
  void set_elt(std::ptrdiff_t i, double x)
  {
    auto b = str->find_block(i);
    RPC_ASSERT(has_block(b));
    rpc::sync(vector_t::set_elt_action(), block(b), i - str->block_begin(b), x);
  }
  std::ostream& output(std::ostream& os) const;
  
  // Level 1
  // TODO: turn these into free functions
  auto faxpy(double alpha, const const_ptr& x) const -> ptr;
  auto fcopy() const -> ptr;
  auto fnrm2() const -> scalar_t::client;
  auto fscal(double alpha) const -> ptr;
  auto fset(double alpha) const -> ptr;
};

inline std::ostream& operator<<(std::ostream& os, const block_vector_t& x)
{
  return x.output(os);
}



class block_matrix_t {
public:
  typedef boost::shared_ptr<const block_matrix_t> const_ptr;
  typedef boost::shared_ptr<block_matrix_t> ptr;
  
  static const int dim = 2;
private:
  friend class block_vector_t;
  std::array<structure_t::const_ptr, dim> strs;
  std::vector<unsigned char> has_block_; // vector<bool> is not thread-safe
  std::vector<rpc::client<matrix_t> > blocks_;
  std::ptrdiff_t linear_blocks() const
  {
    return strs[0]->num_blocks() * strs[1]->num_blocks();
  }
  std::ptrdiff_t linear_block(std::ptrdiff_t bi, std::ptrdiff_t bj) const
  {
    RPC_ASSERT(bi>=0 && bi<strs[0]->num_blocks());
    RPC_ASSERT(bj>=0 && bj<strs[1]->num_blocks());
    return bi + strs[0]->num_blocks() * bj;
  }
  int choose_proc(std::ptrdiff_t bi, std::ptrdiff_t bj) const
  {
    // Round robin
    auto b = linear_block(bi, bj);
    return b % rpc::server->size();
  }
public:
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
  block_matrix_t(const structure_t::const_ptr& istr,
                 const structure_t::const_ptr& jstr,
                 const std::initializer_list<block_t>& blocks_);
  operator std::string() const { return mkstr(*this); }
  std::ptrdiff_t num_blocks(int d) const
  {
    RPC_ASSERT(d>=0 && d<dim);
    return strs[d]->num_blocks();
  }
  std::ptrdiff_t block_begin(int d, std::ptrdiff_t b) const
  {
    RPC_ASSERT(d>=0 && d<dim);
    return strs[d]->block_begin(b);
  }
  std::ptrdiff_t block_size(int d, std::ptrdiff_t b) const
  {
    RPC_ASSERT(d>=0 && d<dim);
    return strs[d]->block_size(b);
  }
  bool has_block(std::ptrdiff_t ib, std::ptrdiff_t jb) const
  {
    auto b = linear_block(ib, jb);
    return has_block_[b];
  }
  void make_block(std::ptrdiff_t ib, std::ptrdiff_t jb);
  void remove_block(std::ptrdiff_t ib, std::ptrdiff_t jb);
  const rpc::client<matrix_t>& block(std::ptrdiff_t ib, std::ptrdiff_t jb) const
  {
    RPC_ASSERT(has_block(ib,jb));
    auto b = linear_block(ib, jb);
    return blocks_[b];
  }
  void set_block(std::ptrdiff_t ib, std::ptrdiff_t jb,
                 const rpc::client<matrix_t>& a)
  {
    // TODO: check that a is not NULL
    auto b = linear_block(ib, jb);
    has_block_[b] = true;
    blocks_[b] = a;
  }
  bool has_elt(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    auto ib = strs[0]->find_block(i);
    auto jb = strs[1]->find_block(j);
    return has_block(ib,jb);
  }
  double operator()(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    auto ib = strs[0]->find_block(i);
    auto jb = strs[1]->find_block(j);
    static const double zero = 0.0;
    if (!has_block(ib,jb)) return zero;
    return rpc::sync(matrix_t::get_elt_action(), block(ib,jb),
                     i - block_begin(0, ib), j - block_begin(1, jb));
  }
  void set_elt(std::ptrdiff_t i, std::ptrdiff_t j, double x)
  {
    auto ib = strs[0]->find_block(i);
    auto jb = strs[1]->find_block(j);
    RPC_ASSERT(has_block(ib,jb));
    rpc::sync(matrix_t::set_elt_action(), block(ib,jb),
              i - block_begin(0, ib), j - block_begin(1, jb), x);
  }
  std::ostream& output(std::ostream& os) const;
  
  // Level 2
  auto faxpy(bool transa, bool transb0,
             double alpha, const const_ptr& a) const -> ptr;
  auto fcopy(bool trans) const -> ptr;
  auto fgemv(bool trans,
             double alpha, const block_vector_t::const_ptr& x,
             double beta, const block_vector_t::const_ptr& y0) const ->
    block_vector_t::ptr;
  auto fnrm2() const -> scalar_t::client;
  auto fscal(bool trans, double alpha) const -> ptr;
  auto fset(bool trans, double alpha) const -> ptr;
  
  // Level 3
  auto fgemm(bool transa, bool transb, bool transc0,
             double alpha, const const_ptr& b,
             double beta, const const_ptr& c0) const -> ptr;
};

inline std::ostream& operator<<(std::ostream& os, const block_matrix_t& a)
{
  return a.output(os);
}

#endif // #ifndef BLOCK_MATRIX_HH
