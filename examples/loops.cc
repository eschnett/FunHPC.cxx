#include <funhpc/async.hpp>
#include <funhpc/main.hpp>

#include <algorithm>
#include <cassert>
#include <vector>

// Synchronize the ghost zones (the outermost points in each direction)
void sync(double *y, int n) {
  // we just assume periodic boundaries
  assert(n >= 2);
  y[0] = y[n - 2];
  y[n - 1] = y[1];
}

// A basic loop, calculating a first derivative
void vdiff(double *y, const double *x, int n) {
  for (int i = 1; i < n - 1; ++i) {
    y[i] = (x[i + 1] - x[i - 1]) / 2;
  }
  sync(y, n);
}

// The same loop, parallelized via OpenMP: The number of iterations is
// split over the available number of cores (e.g. 12 on a NUMA node of
// Wheeler). The disadvantages of this are:
// - There is a implicit barrier at the end of the loop, so that the
//   sync afterwards cannot overlap with the loop
// - A single thread might handle too much work, overflowing the cache
// - A single thread might not have enough work, so that the thread
//   management overhead is too large
void vdiff_openmp(double *y, const double *x, int n) {
#pragma omp parallel for
  for (int i = 1; i < n - 1; ++i) {
    y[i] = (x[i + 1] - x[i - 1]) / 2;
  }
  sync(y, n);
}

// The same loop, this time parallelized via FunHPC. Each thread
// handles a well-defined amount of work, to be chosen based on the
// complexity of the loop kernel.
// - Each thread runs until its result is needed. The interior of the
//   domain can be calculated overlapping with the synchronization.
// - If the number of iterations is small, only a single thread is
//   used. Other cores are free to do other work, e.g. analysis or
//   I/O.
// - If the number of iterations is large, then many threads will be
//   created. The threads will be executed in some arbitrary order.
//   The cost of creating a thread is small (but of not negligible) --
//   there is no problem if thousands of threads are created.
void vdiff_funhpc(double *y, const double *x, int n) {
  // number of points per thread (depending on architecture and cache size)
  // (the number here is much too small; this is just for testing)
  const int blocksize = 8;

  // loop over blocks, starting one thread for each
  std::vector<qthread::future<void>> fs;
  for (int i0 = 1; i0 < n - 1; i0 += blocksize) {
    fs.push_back(qthread::async(qthread::launch::async, [=]() {

      // loop over the work of a single thread
      const int imin = i0;
      const int imax = std::min(i0 + blocksize, n - 1);
      for (int i = imin; i < imax; ++i) {
        y[i] = (x[i + 1] - x[i - 1]) / 2;
      }
    }));
  }

  // synchronize as soon as the boundary results are available
  assert(!fs.empty());
  fs[0].wait();
  fs[fs.size() - 1].wait();
  sync(y, n);

  // wait for all threads to finish
  for (const auto &f : fs)
    f.wait();
}

int funhpc_main(int argc, char **argv) {
  const int n = 1000000;
  std::vector<double> x(n), y(n);
  vdiff(&y[0], &x[0], n);
  vdiff_openmp(&y[0], &x[0], n);
  vdiff_funhpc(&y[0], &x[0], n);
  return 0;
}
