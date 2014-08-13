#include "hwloc.hh"

#include <iostream>

using std::cout;

int rpc_main(int argc, char **argv) {
  cout << "hwloc information:\n";
  hwloc_bindings(false, true);
  cout << "Setting CPU bindings:\n";
  hwloc_bindings(true, true);
  return 0;
}
