#include <funhpc/main.hpp>
#include <funhpc/rexec.hpp>

#include <iostream>

int funhpc_main(int argc, char **argv) {
  std::cout << "Hello, World!\n"
            << "This is FunHPC on " << funhpc::size() << " processes.\n"
            << "Done.\n";
  return 0;
}
