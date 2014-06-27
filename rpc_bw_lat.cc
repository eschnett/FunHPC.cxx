#include "rpc.hh"

#include <boost/serialization/vector.hpp>

#include <cstddef>
#include <iostream>
#include <vector>

using rpc::async;
using rpc::launch;
using rpc::remote;

using std::cout;
using std::flush;
using std::ptrdiff_t;
using std::vector;

unsigned char ping(unsigned char payload) { return payload; }
vector<unsigned char> xfer(vector<unsigned char> payload) { return payload; }
RPC_ACTION(ping);
RPC_ACTION(xfer);

void ping_direct() {
  cout << "   latency direct: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    volatile unsigned char r __attribute__((unused)) = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      unsigned char payload = r;
      r = ping(payload);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 1.0e+9 * elapsed / 2 / n << " nsec   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void ping_local() {
  cout << "   latency local: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    volatile unsigned char r __attribute__((unused)) = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      unsigned char payload = r;
      r = async(launch::async, ping, payload).get();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 1.0e+9 * elapsed / 2 / n << " nsec   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void ping_remote() {
  if (rpc::server->size() == 1)
    return;
  RPC_ASSERT(rpc::server->rank() == 0);
  cout << "   latency remote: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    volatile unsigned char r __attribute__((unused)) = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      unsigned char payload = r;
      r = async(remote::async, 1, ping_action(), payload).get();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 1.0e+9 * elapsed / 2 / n << " nsec   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void xfer_direct(ptrdiff_t sz) {
  RPC_ASSERT(sz > 0);
  cout << "   bandwidth direct: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    unsigned char r = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      r += xfer(payload)[0];
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 2.0 * n *sz / (1.0e+9 * elapsed) << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void xfer_local(ptrdiff_t sz) {
  RPC_ASSERT(sz > 0);
  cout << "   bandwidth local: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    unsigned char r = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      r += async(launch::async, xfer, payload).get()[0];
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 2.0 * n *sz / (1.0e+9 * elapsed) << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void xfer_remote(ptrdiff_t sz) {
  RPC_ASSERT(sz > 0);
  if (rpc::server->size() == 1)
    return;
  RPC_ASSERT(rpc::server->rank() == 0);
  cout << "   bandwidth remote: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n < 1000 * 1000 * 1000; n *= 2) {
    unsigned char r = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      r += async(remote::async, 1, xfer_action(), payload).get()[0];
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= 0.1)
      break;
  }
  cout << 2.0 * n *sz / (1.0e+9 * elapsed) << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

int rpc_main(int argc, char **argv) {
  cout << "Measuring bandwidth and latency:\n";
  ping_direct();
  ping_local();
  ping_remote();
  const ptrdiff_t sz = 1000 * 1000;
  xfer_direct(sz);
  xfer_local(sz);
  xfer_remote(sz);
  return 0;
}
