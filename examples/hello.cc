#include <funhpc/rexec>
#include <funhpc/main>

#include <iostream>

int funhpc_main(int argc, char **argv) {
  std::cout << "Hello, World! This is FunHPC on " << funhpc::size()
            << " processes.\n";
  return 0;
}
