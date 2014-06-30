#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using std::cout;
using std::flush;
using std::memcpy;
using std::min;
using std::numeric_limits;
using std::ptrdiff_t;
using std::string;
using std::stringstream;
using std::vector;

constexpr int recv_buffer_size = 256;

template<typename T>
void send(const boost::mpi::communicator &comm, int dest, int tag, const T& d)
{
  stringstream ss;
  {
    cereal::BinaryOutputArchive oa(ss);
    oa(d);
  }
  string s = std::move(ss).str();
  int sz = s.size();
  if (sz < recv_buffer_size) {
    comm.send(dest, tag, s.data(), sz);
  } else {
    int offset = recv_buffer_size - sizeof sz;
    int saved_bytes;
    memcpy(&saved_bytes, &s[offset], sizeof sz);
    memcpy(&s[offset], &sz, sizeof sz);
    comm.send(dest, tag, &s[0], recv_buffer_size);
    memcpy(&s[offset], &saved_bytes, sizeof sz);
    comm.send(dest, tag, &s[offset], sz - offset);
  }
}

template<typename T>
boost::mpi::status
recv(const boost::mpi::communicator &comm, int src, int tag, T& d)
{
  boost::mpi::status st;
  string s;
  s.resize(recv_buffer_size);
  st = comm.recv(src, tag, &s[0], recv_buffer_size);
  if (st.error()) return st;
  int sz = st.count<char>().get();
  if (sz < recv_buffer_size) {
    s.resize(sz);
  } else {
    assert(sz == recv_buffer_size);
    int offset = recv_buffer_size - sizeof sz;
    memcpy(&sz, &s[offset], sizeof sz);
    s.resize(sz);
    st = comm.recv(src, tag, &s[offset], sz - offset);
    if (st.error()) return st;
  }
  stringstream ss(std::move(s));
  {
    cereal::BinaryInputArchive ia(ss);
    ia(d);
  }
  return st;
}

constexpr double min_elapsed = 1.0;
constexpr ptrdiff_t maxn = numeric_limits<ptrdiff_t>::max();

void ping(const boost::mpi::communicator &comm, ptrdiff_t sz) {
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
        send(comm, 1, 0, payload);
        recv(comm, 1, 0, payload);
        r += payload[0];
      } else {
        recv(comm, 0, 0, payload);
        send(comm, 0, 0, payload);
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
  ping(comm, 1);
  const ptrdiff_t sz = 1000 * 1000;
  ping(comm, sz);
  if (rank == 0) {
    cout << "Done.\n";
  }
  return 0;
}
