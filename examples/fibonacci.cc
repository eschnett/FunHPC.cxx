#include <funhpc/main.hpp>
#include <qthread/future.hpp>

#include <chrono>
#include <iostream>
#include <utility>
#include <vector>

int fib(int n) {
  if (n == 0)
    return 0;
  if (n == 1)
    return 1;
  auto f1 = qthread::async(fib, n - 1);
  auto f2 = qthread::async(fib, n - 2);
  return f1.get() + f2.get();
}

int funhpc_main(int argc, char **argv) {
  std::cout << "Fibonacci\n";
  auto n = 20;
  std::cout << "fib(" << n << ") = " << std::flush;
  auto t0 = std::chrono::high_resolution_clock::now();
  auto f = fib(n);
  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << f;
  std::cout << "   ("
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0)
                   .count() << " ms)\n";
  std::cout << "Done.\n";
  return 0;
}
