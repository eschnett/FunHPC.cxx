#include <funhpc/main>
#include <funhpc/server>

int main(int argc, char **argv) {
  using namespace funhpc;
  initialize(argc, argv);
  auto res = eventloop(funhpc_main, argc, argv);
  finalize();
  return res;
}
