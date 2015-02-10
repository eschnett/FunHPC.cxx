#include <funhpc/main>
#include <funhpc/rexec>
#include <qthread/future>
#include <qthread/thread>

#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>

#include <chrono>
#include <iostream>

// Cannot have global variables with qthread:: types, since Qthreads
// is initialized too late and finalized too early
std::unique_ptr<qthread::promise<void>> p;

void pong() { p->set_value(); }

void ping() { funhpc::rexec(0, pong); }

int funhpc_main(int argc, char **argv) {
  std::cout << "Ping-Pong\n";
  std::size_t count = 1000;
  auto t0 = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < count; ++i) {
    p = std::make_unique<qthread::promise<void>>();
    funhpc::rexec(1 % funhpc::size(), ping);
    p->get_future().wait();
    p.reset();
  }
  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << "   ping-pong time: ("
            << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0)
                       .count() /
                   count << " μs)\n";
  std::cout << "Done.\n";
  return 0;
}
