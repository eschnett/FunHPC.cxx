#include "rpc_thread.hh"
#include "rpc_server.hh"
#include "rpc_server_mpi.hh"

#include <atomic>
#include <cassert>
#include <iostream>

int rpc_main(int argc, char **argv);

namespace rpc {

int real_main(int argc, char **argv) {
  // MPI_Init(&argc, &argv);
  rpc::server = new rpc::server_mpi(argc, argv);
  rpc::thread_initialize();
  std::cout << "[" << rpc::server->rank() << "] "
            << "MPI processes: " << rpc::server->size() << "\n";
  {
    boost::mpi::environment env(argc, argv);
    std::cout << "[" << rpc::server->rank() << "] "
              << "Processor name: " << env.processor_name() << "\n";
  }
  std::cout << "[" << rpc::server->rank() << "] "
            << "Hardware concurrency: " << thread::hardware_concurrency()
            << "\n";
  // Prevent the above info output from cluttering the real program
  // output
  server->barrier();
  if (rpc::server->rank() == 0) {
    std::cout << "Running...\n";
  }
  int iret = rpc::server->event_loop(rpc_main);
  if (rpc::server->rank() == 0) {
    if (iret == 0) {
      std::cout << "Done: success.\n";
    } else {
      std::cout << "Done: failure (error=" << iret << ").\n";
    }
  }
  // Prevent shutdown messages from cluttering the real program output
  server->barrier();
  rpc::thread_finalize();
  delete rpc::server;
  rpc::server = nullptr;
  // MPI_Finalize();
  return iret;
}
}

int main(int argc, char **argv) { return rpc::thread_main(argc, argv); }
