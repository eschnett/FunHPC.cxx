#include <hpx/hpx.hpp>
#include <hpx/hpx_fwd.hpp>
#include <hpx/hpx_main.hpp>

#include <boost/serialization/vector.hpp>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <limits>
#include <vector>

using hpx::async;
using hpx::launch;

using std::cout;
using std::flush;
using std::numeric_limits;
using std::ptrdiff_t;
using std::vector;

unsigned char ping(unsigned char payload) { return payload; }
vector<unsigned char> xfer(vector<unsigned char> payload) { return payload; }
HPX_PLAIN_ACTION(ping, ping_action);
HPX_PLAIN_ACTION(xfer, xfer_action);

constexpr double min_elapsed = 1.0;
constexpr ptrdiff_t maxn = numeric_limits<ptrdiff_t>::max();

void ping_direct() {
  cout << "   latency direct: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
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
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double lat = elapsed / count;
  cout << lat * 1.0e+9 << " nsec   (iterations: " << n << ", time: " << elapsed
       << " s)\n" << flush;
}

void ping_local() {
  cout << "   latency local: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
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
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double lat = elapsed / count;
  cout << lat * 1.0e+9 << " nsec   (iterations: " << n << ", time: " << elapsed
       << " s)\n" << flush;
}

void ping_remote() {
  if (hpx::get_num_localities_sync() == 1)
    return;
  auto other = hpx::find_remote_localities().at(0);
  cout << "   latency remote: " << flush;
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
    volatile unsigned char r __attribute__((unused)) = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      unsigned char payload = r;
      r = async(launch::async, ping_action(), other, payload).get();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double lat = elapsed / count;
  cout << lat * 1.0e+9 << " nsec   (iterations: " << n << ", time: " << elapsed
       << " s)\n" << flush;
}

void xfer_direct(ptrdiff_t sz) {
  assert(sz > 0);
  cout << "   bandwidth direct: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
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
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double bw = 1.0 * sz * count / elapsed;
  cout << bw / 1.0e+9 << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void xfer_local(ptrdiff_t sz) {
  assert(sz > 0);
  cout << "   bandwidth local: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
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
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double bw = 1.0 * sz * count / elapsed;
  cout << bw / 1.0e+9 << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

void xfer_remote(ptrdiff_t sz) {
  assert(sz > 0);
  if (hpx::get_num_localities_sync() == 1)
    return;
  auto other = hpx::find_remote_localities().at(0);
  cout << "   bandwidth remote: " << flush;
  vector<unsigned char> payload(sz, 'a');
  double elapsed;
  ptrdiff_t n;
  for (n = 1; n <= maxn / 2; n *= 2) {
    unsigned char r = 'a';
    auto t0 = std::chrono::high_resolution_clock::now();
    for (ptrdiff_t i = 0; i < n; ++i) {
      r += async(launch::async, xfer_action(), other, payload).get()[0];
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    volatile unsigned char use_r __attribute__((unused)) = r;
    elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
        1.0e+9;
    if (elapsed >= min_elapsed)
      break;
  }
  double count = 2.0 * n;
  double bw = 1.0 * sz * count / elapsed;
  cout << bw / 1.0e+9 << " GByte/s   (iterations: " << n
       << ", time: " << elapsed << " s)\n" << flush;
}

int main(int argc, char **argv) {
  cout << "Measuring bandwidth and latency:\n";
  ping_direct();
  ping_local();
  ping_remote();
  const ptrdiff_t sz = 1000 * 1000;
  xfer_direct(sz);
  xfer_local(sz);
  xfer_remote(sz);
  cout << "Done.\n";
  return 0;
}
