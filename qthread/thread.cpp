#include "thread.hpp"

#include <qthread/future.hpp>

#include <atomic>
#include <vector>

namespace qthread {

namespace all_threads {

void run(const std::function<void()> &f) {
  const unsigned int overload_factor = 10;
  const unsigned int nthreads = qthread::thread::hardware_concurrency();

  std::vector<qthread::future<void>> fs;
  std::atomic<unsigned int> count_begin{0}, count_done{0};
  for (unsigned int i = 0; i < overload_factor * nthreads; ++i)
    fs.push_back(qthread::async(qthread::launch::async, [&]() {
      // Take a sequence number
      const auto mycount = count_begin++;
      // If we're already done, exit early
      if (mycount >= nthreads)
        return;
      // Wait until all hardware threads are here
      while (count_begin < nthreads)
        ;
      // Do the work
      f();
      // Register our work as done
      ++count_done;
      // Wait until all hardware threads are done before leaving
      while (count_done < nthreads)
        ;
    }));

  for (auto &f : fs)
    f.get();

  assert(count_done == nthreads);
}
} // namespace all_threads
} // namespace qthread
