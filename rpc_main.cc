#include "rpc_thread.hh"
#include "rpc_server.hh"
#include "rpc_server_mpi.hh"

#include <cassert>
#include <iostream>

int rpc_main(int argc, char** argv);

namespace rpc {
  
  int real_main(int argc, char** argv)
  {
    rpc::server = new rpc::server_mpi(argc, argv);
    rpc::thread_initialize();
    if (rpc::server->rank() == 0) {
      std::cout << "Running...\n";
    }
    std::cout << "MPI processes: " << rpc::server->size() << "\n";
    std::cout << "[" << rpc::server->rank() << "] Hardware concurrency: "
              << thread::hardware_concurrency() << "\n";
    int iret = rpc::server->event_loop(rpc_main);
    if (rpc::server->rank() == 0) {
      if (iret == 0) {
        std::cout << "Done: success.\n";
      } else {
        std::cout << "Done: failure (error=" << iret << ").\n";
      }
    }
    rpc::thread_finalize();
    delete rpc::server;
    rpc::thread_finalize2();
    return iret;
  }
  
}

int main(int argc, char** argv)
{
  return rpc::thread_main(argc, argv);
}
