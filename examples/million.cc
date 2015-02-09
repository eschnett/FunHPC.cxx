#include <funhpc/main>

#include <qthread/future>

#include <chrono>
#include <iostream>
#include <utility>
#include <vector>

int funhpc_main(int argc, char **argv) {
  std::cout << "One Million Threads\n";
  std::chrono::high_resolution_clock clock;
  std::size_t count = 1000000;
  std::vector<qthread::future<void>> fs(count);
  {
    std::cout << "   starting..." << std::flush;
    auto t0 = clock.now();
    for (auto &f : fs)
      f = qthread::async([]() {});
    auto t1 = clock.now();
    std::cout << " ("
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                     .count() << " ms)\n";
  }
  {
    std::cout << "   waiting..." << std::flush;
    auto t0 = clock.now();
    for (auto &f : fs)
      f.wait();
    auto t1 = clock.now();
    std::cout << " ("
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                     .count() << " ms)\n";
  }
  std::cout << "Done.\n";
  return 0;
}
