#ifndef MATRIX_HH
#define MATRIX_HH

#include "rpc.hh"

#include <boost/serialization/vector.hpp>

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

template <typename T> std::string mkstr(const T &x) {
  std::ostringstream os;
  os << x;
  return os.str();
}

struct scalar_t;
struct vector_t;
struct matrix_t;

struct scalar_t {
  typedef rpc::shared_ptr<const scalar_t> const_ptr;
  typedef rpc::shared_ptr<scalar_t> ptr;
  // TODO: remove all global_ptr?
  // typedef rpc::global_shared_ptr<scalar_t> global_ptr;
  typedef rpc::client<scalar_t> client;

  double elts;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int version) {
    ar &elts;
  }

public:
  explicit scalar_t() {}
  scalar_t(const double &elts_) : elts(elts_) {}
  scalar_t(const scalar_t &) = default;
  scalar_t &operator=(const scalar_t &) {
    RPC_ASSERT(0);
    __builtin_unreachable();
  }
  operator double() const { return elts; }

  operator std::string() const { return mkstr(*this); }
  const double &operator()() const {
    return elts;
    // return *(const double *__restrict__)&elts;
  }
  double &operator()() {
    return elts;
    // return *(double *__restrict__)&elts;
  }
  double get_elt() const { return (*this)(); }
  RPC_DECLARE_CONST_MEMBER_ACTION(scalar_t, get_elt);
  void set_elt(double alpha) { (*this)() = alpha; }
  RPC_DECLARE_MEMBER_ACTION(scalar_t, set_elt);
};
RPC_DECLARE_COMPONENT(scalar_t);

std::ostream &operator<<(std::ostream &os, const scalar_t &x);

struct vector_t {
  typedef rpc::shared_ptr<const vector_t> const_ptr;
  typedef rpc::shared_ptr<vector_t> ptr;
  typedef rpc::global_shared_ptr<vector_t> global_ptr;
  typedef rpc::client<vector_t> client;

  std::ptrdiff_t N;
  std::vector<double> elts;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int version) {
    ar &N;
    ar &elts;
    // TODO
    RPC_ASSERT(N == elts.size());
  }

public:
  explicit vector_t(std::ptrdiff_t N) : N(N), elts(N) {}
  template <typename T> explicit vector_t(std::ptrdiff_t N, const T &elts_);
  // We don't really want these
  vector_t() : N(-1) {}
  vector_t(const vector_t &) = default;
  vector_t &operator=(const vector_t &) {
    RPC_ASSERT(0);
    __builtin_unreachable();
  }

  operator std::string() const { return mkstr(*this); }
  const double &operator()(std::ptrdiff_t i) const {
    RPC_ASSERT(i >= 0 && i < N);
    return elts[i];
    // return *(const double *__restrict__)&elts[i];
  }
  double &operator()(std::ptrdiff_t i) {
    RPC_ASSERT(i >= 0 && i < N);
    return elts[i];
    // return *(double *__restrict__)&elts[i];
  }
  double get_elt(std::ptrdiff_t i) const { return (*this)(i); }
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, get_elt);
  void set_elt(std::ptrdiff_t i, double x) { (*this)(i) = x; }
  RPC_DECLARE_MEMBER_ACTION(vector_t, set_elt);

  // Level 1
  auto faxpy(double alpha, const const_ptr &x) const -> ptr;
  auto fcopy() const -> ptr;
  auto fnrm2() const -> scalar_t::ptr;
  auto fscal(double alpha) const -> ptr;
  auto fset(double alpha) const -> ptr;

  auto gfaxpy(double alpha, global_ptr x) const -> global_ptr {
    auto xloc = x.make_local();
    return faxpy(alpha, xloc.get());
  }
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, gfaxpy);

  // TODO: make actions capture copies intead of const references,
  // then update signatures of functions underlying actions
  auto cfaxpy(double alpha, client x) const -> client {
    auto xloc = x.make_local();
    return faxpy(alpha, xloc.get());
  }
  auto cfcopy() const -> client { return fcopy(); }
  auto cfnrm2() const -> scalar_t::client { return fnrm2(); }
  auto cfscal(double alpha) const -> client { return fscal(alpha); }
  auto cfset(double alpha) const -> client { return fset(alpha); }

  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, cfaxpy);
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, cfcopy);
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, cfnrm2);
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, cfscal);
  RPC_DECLARE_CONST_MEMBER_ACTION(vector_t, cfset);
};
RPC_DECLARE_COMPONENT(vector_t);

std::ostream &operator<<(std::ostream &os, const vector_t &x);

// TODO: put these into rpc::client<...>?
// TODO: allow calling async on rpc::global_shared_ptr<...>?
inline auto afaxpy(double alpha, const vector_t::client &x,
                   const vector_t::client &y0) -> vector_t::client {
  return vector_t::client(
      rpc::async(rpc::remote::async, vector_t::cfaxpy_action(), y0, alpha, x));
  // TODO: Try this instead, measure performance -- it has fewer
  // asyncs, but also exposes less parallelism
  // return rpc::async(rpc::remote::async, y0.get_proc(),
  //                   vector_t::gfaxpy_action(),
  //                   y0.get_global_shared(),
  //                   alpha, x.get_global_shared()));
}
inline auto afcopy(const vector_t::client &x0) -> vector_t::client {
  return vector_t::client(
      rpc::async(rpc::remote::async, vector_t::cfcopy_action(), x0));
}
inline auto afnrm2(const vector_t::client &x0) -> scalar_t::client {
  return scalar_t::client(
      rpc::async(rpc::remote::async, vector_t::cfnrm2_action(), x0));
}
inline auto afscal(double alpha, const vector_t::client &x0)
    -> vector_t::client {
  return vector_t::client(
      rpc::async(rpc::remote::async, vector_t::cfscal_action(), x0, alpha));
}
inline auto afset(double alpha, const vector_t::client &x0)
    -> vector_t::client {
  return vector_t::client(
      rpc::async(rpc::remote::async, vector_t::cfset_action(), x0, alpha));
}

struct matrix_t {
  typedef rpc::shared_ptr<const matrix_t> const_ptr;
  typedef rpc::shared_ptr<matrix_t> ptr;
  // typedef rpc::global_shared_ptr<matrix_t> global_ptr;
  typedef rpc::client<matrix_t> client;

  static std::atomic<std::ptrdiff_t> elts_sent, elts_received;
  static std::atomic<std::ptrdiff_t> objs_sent, objs_received;
  static void reset_stats();
  RPC_DECLARE_ACTION(reset_stats);
  static std::string output_stats();
  RPC_DECLARE_ACTION(output_stats);

  std::ptrdiff_t NI, NJ; // interpretation: row, column
  std::vector<double> elts;

private:
  friend class boost::serialization::access;
  template <class Archive> void save(Archive &ar, unsigned int version) const {
    ar &NI &NJ;
    ar &elts;
    elts_sent += NI * NJ;
    ++objs_sent;
  }
  template <class Archive> void load(Archive &ar, unsigned int version) {
    ar &NI &NJ;
    ar &elts;
    elts_received += NI * NJ;
    ++objs_received;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER();

public:
  explicit matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ)
      : NI(NI), NJ(NJ), elts(NI * NJ) {}
  template <typename T>
  explicit matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ, const T &elts_);
  // We don't really want these
  matrix_t() : NI(-1), NJ(-1) {}
  matrix_t(const matrix_t &) = default;

  operator std::string() const { return mkstr(*this); }
  const double &operator()(std::ptrdiff_t i, std::ptrdiff_t j) const {
    RPC_ASSERT(i >= 0 && i < NI && j >= 0 && j < NJ);
    return elts[i + NI * j];
    // return *(const double *__restrict__)&elts[i+NI*j];
  }
  double &operator()(std::ptrdiff_t i, std::ptrdiff_t j) {
    RPC_ASSERT(i >= 0 && i < NI && j >= 0 && j < NJ);
    return elts[i + NI * j];
    // return *(double *__restrict__)&elts[i+NI*j];
  }
  double get_elt(std::ptrdiff_t i, std::ptrdiff_t j) const {
    return (*this)(i, j);
  }
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, get_elt);
  void set_elt(std::ptrdiff_t i, std::ptrdiff_t j, double x) {
    (*this)(i, j) = x;
  }
  RPC_DECLARE_MEMBER_ACTION(matrix_t, set_elt);

  // Level 2
  auto faxpy(bool transa, bool transb0, double alpha,
             const const_ptr &a) const -> ptr;
  auto fgemv(bool trans, double alpha, const vector_t::const_ptr &x,
             double beta, const vector_t::const_ptr &y0) const -> vector_t::ptr;
  auto fcopy(bool trans) const -> ptr;
  auto fnrm2() const -> scalar_t::ptr;
  auto fscal(bool trans, double alpha) const -> ptr;
  auto fset(bool trans, double alpha) const -> ptr;

  // Level 3
  auto fgemm(bool transa, bool transb, bool transc0, double alpha,
             const const_ptr &b, double beta, const const_ptr &c0) const -> ptr;

  auto cfaxpy(bool transa, bool transb0, double alpha,
              client a) const -> client {
    auto aloc = a.make_local();
    return faxpy(transa, transb0, alpha, aloc.get());
  }
  auto cfcopy(bool trans) const -> client { return fcopy(trans); }
  auto cfgemv(bool trans, double alpha, vector_t::client x, double beta,
              vector_t::client y0) const -> vector_t::client {
    auto xloc = x.make_local();
    auto y0loc = y0.make_local();
    return fgemv(trans, alpha, xloc.get(), beta, y0loc.get());
  }
  auto cfnrm2() const -> scalar_t::client { return fnrm2(); }
  auto cfscal(bool trans, double alpha) const -> client {
    return fscal(trans, alpha);
  }
  auto cfset(bool trans, double alpha) const -> client {
    return fset(trans, alpha);
  }
  auto cfgemm(bool transa, bool transb, bool transc0, double alpha, client b,
              double beta, client c0) const -> client {
    auto bloc = b.make_local();
    auto c0loc = c0.make_local();
    return fgemm(transa, transb, transc0, alpha, bloc.get(), beta, c0loc.get());
  }

  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfaxpy);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfcopy);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfgemv);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfnrm2);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfscal);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfset);
  RPC_DECLARE_CONST_MEMBER_ACTION(matrix_t, cfgemm);
};
RPC_DECLARE_COMPONENT(matrix_t);

std::ostream &operator<<(std::ostream &os, const matrix_t &a);

inline auto afaxpy(bool transa, bool transb0, double alpha,
                   const matrix_t::client &a,
                   const matrix_t::client &b0) -> matrix_t::client {
  return matrix_t::client(rpc::async(rpc::remote::async,
                                     matrix_t::cfaxpy_action(), b0, transa,
                                     transb0, alpha, a));
}
inline auto afcopy(bool trans, const matrix_t::client &a0) -> matrix_t::client {
  return matrix_t::client(
      rpc::async(rpc::remote::async, matrix_t::cfcopy_action(), a0, trans));
}
inline auto afgemv(bool trans, double alpha, matrix_t::client a,
                   vector_t::client x, double beta,
                   vector_t::client y0) -> vector_t::client {
  return vector_t::client(rpc::async(rpc::remote::async,
                                     matrix_t::cfgemv_action(), a, trans, alpha,
                                     x, beta, y0));
}
inline auto afnrm2(const matrix_t::client &a0) -> scalar_t::client {
  return scalar_t::client(
      rpc::async(rpc::remote::async, matrix_t::cfnrm2_action(), a0));
}
inline auto afscal(bool trans, double alpha, const matrix_t::client &a0)
    -> matrix_t::client {
  return matrix_t::client(rpc::async(
      rpc::remote::async, matrix_t::cfscal_action(), a0, trans, alpha));
}
inline auto afset(bool trans, double alpha, const matrix_t::client &a0)
    -> matrix_t::client {
  return matrix_t::client(rpc::async(
      rpc::remote::async, matrix_t::cfset_action(), a0, trans, alpha));
}
inline auto afgemm(bool transa, bool transb, bool transc0, double alpha,
                   const matrix_t::client &a, const matrix_t::client &b,
                   double beta,
                   const matrix_t::client &c0) -> matrix_t::client {
  return matrix_t::client(rpc::async(rpc::remote::async,
                                     matrix_t::cfgemm_action(), a, transa,
                                     transb, transc0, alpha, b, beta, c0));
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
vector_t::vector_t(std::ptrdiff_t N, const T &elts_)
    : N(N), elts(N) {
  std::ptrdiff_t i = 0;
  for (const auto elt : elts_) {
    (*this)(i) = elt;
    ++i;
  }
  RPC_ASSERT(i == N);
}

template <typename T>
matrix_t::matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ, const T &elts_)
    : NI(NI), NJ(NJ), elts(NI * NJ) {
  std::ptrdiff_t i = 0;
  for (const auto &row : elts_) {
    std::ptrdiff_t j = 0;
    while (j < NJ) {
      const auto elt = row[j];
      (*this)(i, j) = elt;
      ++j;
    }
    RPC_ASSERT(j == NJ);
    ++i;
  }
  RPC_ASSERT(i == NI);
}

#endif // #ifndef MATRIX_HH
