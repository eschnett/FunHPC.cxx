#include "hpx.hh"

#include <hpx/hpx_init.hpp>

#include "cxx_utils.hh"

#include <mpi.h>

#include <cassert>
#include <iostream>

using std::cout;

namespace rpc {
int real_main(int argc, char **argv) { RPC_ASSERT(0); }
}

void outx(int x) { cout << "x=" << x << "\n"; }

int rpc_main(int argc, char **argv) {
  // MPI_Init(&argc, &argv);
  auto f = hpx::async(outx, 1);
  f.wait();
  hpx::thread(outx, 1).detach();
  hpx::this_thread::sleep_for(boost::chrono::seconds(1));
  // MPI_Finalize();
  return 0;
}

int hpx_main(int argc, char **argv) {
  int iret = rpc_main(argc, argv);
  hpx::finalize();
  return iret;
}

int main(int argc, char **argv) { return hpx::init(argc, argv); }
