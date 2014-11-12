#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <iostream>
#include <utility>
using namespace hpx;
using namespace std;

int main(int argc, char **argv) {
  cout << "Starting...\n";
  future<future<void> > f1 =
      async(launch::deferred, []() { return async([]() {}); });
  future<void> f2(move(f1));
  f2.wait();
  cout << "Done.\n";
  return 0;
}
