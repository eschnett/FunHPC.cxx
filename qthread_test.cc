#include "qthread.hh"

#include <mpi.h>

#include <iostream>

using std::cout;



void outx(int x) { cout << "x=" << x << "\n"; }

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  qthread::initialize();
  auto f = qthread::async(outx, 1);
  f.wait();
  qthread::finalize();
  MPI_Finalize();
  return 0;
}
