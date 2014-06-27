#include <mpi.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

using std::cout;
using std::flush;
using std::numeric_limits;
using std::ptrdiff_t;
using std::vector;

constexpr ptrdiff_t maxn = numeric_limits<ptrdiff_t>::max();

void ping(ptrdiff_t sz) {
  assert(sz >= 1);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    if (sz == 1) {
      cout << "   latency: " << flush;
    } else {
      cout << "   bandwidth: " << flush;
    }
  }
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
    vector<unsigned char> payload(sz, 'a');
    unsigned char r = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      if (rank == 0) {
        MPI_Send(&payload[0], sz, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&payload[0], sz, MPI_UNSIGNED_CHAR, 1, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        r += payload[0];
      } else {
        MPI_Recv(&payload[0], sz, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        MPI_Send(&payload[0], sz, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    int done = elapsed >= 0.1;
    MPI_Bcast(&done, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (done)
      break;
  }
  if (rank == 0) {
    if (sz == 1) {
      double count = 2.0 * n;
      double lat = elapsed / count;
      cout << lat * 1.0e+9 << " nsec   (iterations: " << n
           << ", time: " << elapsed << " s)\n" << flush;
    } else {
      double count = 2.0 * n;
      double bw = 1.0 * sz * count / elapsed;
      cout << bw / 1.0e+9 << " GByte/s   (iterations: " << n
           << ", time: " << elapsed << " s)\n" << flush;
    }
  }
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  assert(size == 2);
  if (rank == 0) {
    cout << "Measuring bandwidth and latency:\n";
  }
  ping(1);
  const ptrdiff_t sz = 1000 * 1000;
  ping(sz);
  if (rank == 0) {
    cout << "Done.\n";
  }
  MPI_Finalize();
  return 0;
}
