#include "qthread.hh"

#include <iostream>

using std::cout;



void outx(int x) { cout << "x=" << x << "\n"; }

int main(int argc, char** argv)
{
  qthread::initialize();
  auto f = qthread::async(outx, 1);
  f.wait();
  qthread::finalize();
  return 0;
}
