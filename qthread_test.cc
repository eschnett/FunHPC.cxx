#include "qthread.hh"

#include <mpi.h>

#include <cassert>
#include <iostream>

using std::cout;



namespace rpc {
  int real_main(int argc, char** argv) { assert(0); }
}

void outx(int x) { cout << "x=" << x << "\n"; }

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  rpc::thread_initialize();
  auto f = rpc::async(outx, 1);
  f.wait();
  rpc::thread_finalize();
  MPI_Finalize();
  return 0;
}
