#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>

#include <iostream>

using std::cout;



void outx(int x) { cout << "x=" << x << "\n"; }

int hpx(int argc, char** argv)
{
  cout << "main1\n";
  auto f = hpx::async(outx, 1);
  cout << "main2\n";
  f.wait();
  cout << "main3\n";
  return 0;
}
