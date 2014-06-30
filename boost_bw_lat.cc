#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/serialization/vector.hpp>

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

constexpr double min_elapsed = 1.0;
constexpr ptrdiff_t maxn = numeric_limits<ptrdiff_t>::max();

void ping_direct(const boost::mpi::communicator &comm, ptrdiff_t sz) {
  assert(sz >= 1);
  int rank = comm.rank();
  if (rank == 0) {
    if (sz == 1) {
      cout << "   latency (direct): " << flush;
    } else {
      cout << "   bandwidth (direct): " << flush;
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
        comm.send(1, 0, &payload[0], sz);
        comm.recv(1, 0, &payload[0], sz);
        r += payload[0];
      } else {
        comm.recv(0, 0, &payload[0], sz);
        comm.send(0, 0, &payload[0], sz);
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    bool done = elapsed >= min_elapsed;
    boost::mpi::broadcast(comm, done, 0);
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

void ping_direct2(const boost::mpi::communicator &comm, ptrdiff_t sz) {
  assert(sz >= 1);
  int rank = comm.rank();
  if (rank == 0) {
    if (sz == 1) {
      cout << "   latency (direct2): " << flush;
    } else {
      cout << "   bandwidth (direct2): " << flush;
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
        comm.send(1, 0, &sz, 1);
        comm.send(1, 0, &payload[0], sz);
        comm.recv(1, 0, &sz, 1);
        payload.resize(sz);
        comm.recv(1, 0, &payload[0], sz);
        r += payload[0];
      } else {
        comm.recv(0, 0, &sz, 1);
        payload.resize(sz);
        comm.recv(0, 0, &payload[0], sz);
        comm.send(0, 0, &sz, 1);
        comm.send(0, 0, &payload[0], sz);
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    bool done = elapsed >= min_elapsed;
    boost::mpi::broadcast(comm, done, 0);
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

void ping_serialize(const boost::mpi::communicator &comm, ptrdiff_t sz) {
  assert(sz >= 1);
  int rank = comm.rank();
  if (rank == 0) {
    if (sz == 1) {
      cout << "   latency (serializing): " << flush;
    } else {
      cout << "   bandwidth (serializing): " << flush;
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
        comm.send(1, 0, payload);
        comm.recv(1, 0, payload);
        r += payload[0];
      } else {
        comm.recv(0, 0, payload);
        comm.send(0, 0, payload);
      }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    bool done = elapsed >= min_elapsed;
    boost::mpi::broadcast(comm, done, 0);
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
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator comm(MPI_COMM_WORLD, boost::mpi::comm_duplicate);
  int size = comm.size();
  int rank = comm.rank();
  assert(size == 2);
  if (rank == 0) {
    cout << "Measuring bandwidth and latency:\n";
  }
  ping_direct(comm, 1);
  ping_direct2(comm, 1);
  ping_serialize(comm, 1);
  const ptrdiff_t sz = 1000 * 1000;
  ping_direct(comm, sz);
  ping_direct2(comm, sz);
  ping_serialize(comm, sz);
  if (rank == 0) {
    cout << "Done.\n";
  }
  return 0;
}
