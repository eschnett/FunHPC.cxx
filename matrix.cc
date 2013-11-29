#include "matrix.hh"



std::ostream& operator<<(std::ostream& os, const vector_t& x)
{
  os << "[";
  for (std::ptrdiff_t i=0; i<x.N; ++i) {
    if (i != 0) os << ",";
    os << x(i);
  }
  os << "]";
  return os;
}



std::ostream& operator<<(std::ostream& os, const matrix_t& a)
{
  os << "[";
  for (std::ptrdiff_t i=0; i<a.NI; ++i) {
    if (i != 0) os << ",";
    os << "[";
    for (std::ptrdiff_t j=0; j<a.NJ; ++j) {
      if (j != 0) os << ",";
      os << a(i,j);
    }
    os << "]";
  }
  os << "]";
  return os;
}
