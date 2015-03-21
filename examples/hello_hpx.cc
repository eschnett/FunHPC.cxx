#include <hpx/hpx.hpp>

#include <iostream>

int hpx_main(int argc, char **argv) {
  std::cout << "Hello, World!\n"
            << "This is HPX.\n"
            << "Done.\n";
  return hpx::finalize(0);
}

int main(int argc, char **argv) { return hpx::init(argc, argv); }
