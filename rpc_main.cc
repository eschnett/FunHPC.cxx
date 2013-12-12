#include "rpc_server.hh"
#include "rpc_server_mpi.hh"

#include <cassert>
#include <iostream>

int rpc_main(int argc, char** argv);

int main(int argc, char** argv)
{
  rpc::server = new rpc::server_mpi(argc, argv);
  int iret = rpc::server->event_loop(rpc_main);
  if (rpc::server->rank() == 0) {
    if (iret == 0) {
      std::cout << "Done: success.\n";
    } else {
      std::cout << "Done: failure (error=" << iret << ").\n";
    }
  }
  delete rpc::server;
  return iret;
}
