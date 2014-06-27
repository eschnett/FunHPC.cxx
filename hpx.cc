#include "hpx.hh"

#include <hpx/hpx_init.hpp>
#include <hpx/include/performance_counters.hpp>

#include <iostream>
#include <string>

namespace rpc {
int real_main(int argc, char **argv);
}

int hpx_main(int argc, char **argv) {
  std::cout << "HPX: localities: " << hpx::get_num_localities_sync() << ", "
            << "OS threads: " << hpx::get_os_thread_count() << "\n";
  // hpx::parcelset::disable d;
  int iret = rpc::real_main(argc, argv);
  hpx::finalize();
  return iret;
}

namespace hpx {

namespace {
std::ptrdiff_t get_thread_counter_value(const std::string &name) {
  auto counter = performance_counters::get_counter(
      (std::string("/threads/count/") + name + std::string("/locality#0/total"))
          .c_str());
  auto value =
      performance_counters::stubs::performance_counter::get_value(counter);
  return value.get_value<std::ptrdiff_t>();
}
}

thread_stats_t get_thread_stats() {
  return { get_thread_counter_value("instantaneous/all"),
           get_thread_counter_value("cumulative") };
}

int thread_main(int argc, char **argv) { return hpx::init(argc, argv); }

void thread_initialize() {}

void thread_finalize() {}
}
