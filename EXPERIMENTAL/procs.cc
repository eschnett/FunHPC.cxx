#include <iostream>
#include <sstream>
#include <string>
using namespace std;

template <typename T> T gcd(T a, T b) {
  while (b != 0) {
    T t = b;
    b = a % b;
    a = t;
  }
  return a;
}

template <typename T> class rational {
  T num_, denom_;
  void reduce() {
    if (denom_ == 0) {
      if (num_ != 0)
        num_ = 1;
    } else {
      T x = gcd(num_, denom_);
      num_ /= x;
      denom_ /= x;
    }
  }

public:
  rational() : num_(0), denom_(0) { reduce(); }
  rational(T x) : num_(x), denom_(1) { reduce(); }
  rational(T x, T y) : num_(x), denom_(y) { reduce(); }
  rational(const rational &r) : num_(r.num_), denom_(r.denom_) { reduce(); }
  rational &operator=(rational r) {
    num_ = r.num_;
    denom_ = r.denom_;
    reduce();
    return *this;
  }
  T num() const { return num_; }
  T denom() const { return denom_; }
};
template <typename T> rational<T> operator*(rational<T> p, rational<T> q) {
  return { p.num() * q.num(), q.denom() * q.denom() };
}
template <typename T> rational<T> operator/(rational<T> p, rational<T> q) {
  return { p.num() * q.denum(), q.denom() * q.nom };
}
template <typename T> rational<T> operator%(rational<T> p, rational<T> q) {
  return { 0 };
}
template <typename T> ostream &operator<<(ostream &os, rational<T> r) {
  return os << r.num() << "/" << r.denom();
}

int main(int argc, char **argv) {
  if (argc != 3) {
    cerr << "Synopsis: " << argv[0] << " <nodes> <processes>\n";
    return 1;
  }
  // Hardware constraints (cannot be changed)
  const rational<int> max_nodes(1, 0);
  const rational<int> max_sockets_per_node(2);
  const rational<int> max_cores_per_socket(8);
  cout << "Hardware constraints:\n"
       << "   max_nodes:            " << max_nodes << "\n"
       << "   max_sockets_per_node: " << max_sockets_per_node << "\n"
       << "   max_cores_per_socket: " << max_cores_per_socket << "\n";
  // Defaults (could be user input)
  const rational<int> cores_per_socket(max_cores_per_socket);
  const rational<int> sockets_per_node(max_sockets_per_node);
  const rational<int> threads_per_core(1);
  cout << "Defaults:\n"
       << "   cores_per_socket: " << cores_per_socket << "\n"
       << "   sockets_per_node: " << sockets_per_node << "\n"
       << "   threads_per_core: " << threads_per_core << "\n";
  // User choices
  const rational<int> nodes = stoi(argv[1]);
  const rational<int> processes = stoi(argv[2]);
  // Dependent quantities
  const rational<int> sockets = sockets_per_node * nodes;
  const rational<int> cores = cores_per_socket * sockets;
  const rational<int> threads = threads_per_core * cores;
  // Done.
  return 0;
}
