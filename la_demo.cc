#include "rpc.hh"

#include "la_dense.hh"
#include "la_blocked.hh"

#include <iostream>

using namespace la;
using namespace rpc;
using namespace std;

int rpc_main(int argc, char **argv) {
  const ptrdiff_t n = 100;
  const ptrdiff_t nb = 10;
  cout << "n = " << n << ", nb = " << nb << "\n";
  blocked_matrix_t a(nb, nb);
  blocked_matrix_t b(nb, nb);
  for (ptrdiff_t jb = 0; jb < nb; ++jb) {
    for (ptrdiff_t ib = 0; ib < nb; ++ib) {
      matrix_t ab(n / nb, n / nb);
      matrix_t bb(n / nb, n / nb);
      for (ptrdiff_t j = 0; j < n / nb; ++j) {
        for (ptrdiff_t i = 0; i < n / nb; ++i) {
          ab.set(i, j, 1.0 * (i + j));
          bb.set(i, j, 1.0 * (i + j + 1));
        }
      }
      // TODO: use make_remote_client
      a.set(ib, jb, make_client<matrix_t>(ab));
      b.set(ib, jb, make_client<matrix_t>(bb));
    }
  }
  cout << "||A|| = " << fnormmb(a) << "\n";
  cout << "||B|| = " << fnormmb(b) << "\n";
  blocked_matrix_t c = gemmb(a, b, 1.0, false, false);
  cout << "||C|| = " << fnormmb(c) << "\n";
  return 0;
}
