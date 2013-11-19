#include "rpc_main.hh"

#include "rpc_server.hh"
#include "rpc_server_mpi.hh"

#include <cassert>

int rpc_main(int argc, char** argv);

int main(int argc, char** argv)
{
  rpc::server = new rpc::server_mpi(argc, argv);
  int iret = rpc::server->event_loop(rpc_main);
  delete rpc::server;
  return iret;
}
