#include "qthread.hh"

#include <mpi.h>

#include <cassert>
#include <iostream>

using qthread::future;
using qthread::shared_future;

using std::cout;
using std::vector;

namespace rpc {
int real_main(int argc, char **argv) { RPC_ASSERT(0); }
}

void outx(int x) { cout << "x=" << x << "\n"; }

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  rpc::thread_initialize();
  auto f = rpc::async(outx, 1);
  f.wait();

#if 0
  vector<future<int> > vi;
  when_all(vi.begin(), vi.end());
  
  vector<future<void> > vv;
  when_all(vv.begin(), vv.end());
  
  when_all(future<int>(), shared_future<double>());
  
  when_all(future<void>(), shared_future<void>());
#endif

  rpc::thread_finalize();
  MPI_Finalize();
  return 0;
}
