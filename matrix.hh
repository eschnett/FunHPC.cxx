#ifndef MATRIX_HH
#define MATRIX_HH

#include <boost/serialization/vector.hpp>
#include <boost/shared_ptr.hpp>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>



template<typename T>
std::string mkstr(const T& x)
{
  std::ostringstream os;
  os << x;
  return os.str();
}



struct vector_t {
  typedef boost::shared_ptr<const vector_t> const_ptr;
  typedef boost::shared_ptr<vector_t> ptr;
  
  std::ptrdiff_t N;
  std::vector<double> elts;
  
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, unsigned int version)
  {
    ar & N;
    ar & elts;
  }
public:
  
  explicit vector_t(std::ptrdiff_t N): N(N), elts(N) {}
  template<typename T>
  explicit vector_t(std::ptrdiff_t N, const T& elts_);
  // We don't really want these
  vector_t(): N(-1) {}
  vector_t(const vector_t&) = default;
  vector_t& operator=(const vector_t&) { assert(0); __builtin_unreachable(); }
  
  operator std::string() const { return mkstr(*this); }
  const double& operator()(std::ptrdiff_t i) const
  {
    assert(i>=0 && i<N);
    return elts[i];
    // return ((const double *__restrict__)&elts[0])[i];
  }
  double& operator()(std::ptrdiff_t i)
  {
    assert(i>=0 && i<N);
    return elts[i];
    // return ((double *__restrict__)&elts[0])[i];
  }
};

std::ostream& operator<<(std::ostream& os, const vector_t& x);



struct matrix_t {
  typedef boost::shared_ptr<const matrix_t> const_ptr;
  typedef boost::shared_ptr<matrix_t> ptr;
  
  std::ptrdiff_t NI, NJ;        // interpretation: row, column
  std::vector<double> elts;
  
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, unsigned int version)
  {
    ar & NI & NJ;
    ar & elts;
  }
public:
  
  explicit matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ):
    NI(NI), NJ(NJ), elts(NI*NJ)
  {
  }
  template<typename T>
  explicit matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ, const T& elts_);
  // We don't really want these
  matrix_t(): NI(-1), NJ(-1) {}
  matrix_t(const matrix_t&) = default;
  
  operator std::string() const { return mkstr(*this); }
  const double& operator()(std::ptrdiff_t i, std::ptrdiff_t j) const
  {
    assert(i>=0 && i<NI && j>=0 && j<NJ);
    return elts[i+NI*j];
  }
  double& operator()(std::ptrdiff_t i, std::ptrdiff_t j)
  {
    assert(i>=0 && i<NI && j>=0 && j<NJ);
    return elts[i+NI*j];
  }
};

std::ostream& operator<<(std::ostream& os, const matrix_t& a);



////////////////////////////////////////////////////////////////////////////////

template<typename T>
vector_t::vector_t(std::ptrdiff_t N, const T& elts_): N(N), elts(N)
{
  std::ptrdiff_t i = 0;
  for (const auto elt: elts_) {
    (*this)(i) = elt;
    ++i;
  }
  assert(i == N);
}

template<typename T>
matrix_t::matrix_t(std::ptrdiff_t NI, std::ptrdiff_t NJ, const T& elts_):
  NI(NI), NJ(NJ), elts(NI*NJ)
{
  std::ptrdiff_t i = 0;
  for (const auto& row: elts_) {
    std::ptrdiff_t j = 0;
    while (j < NJ) {
      const auto elt = row[j];
      (*this)(i,j) = elt;
      ++j;
    }
    assert(j == NJ);
    ++i;
  }
  assert(i == NI);
}

#endif // #ifndef MATRIX_HH
