#include "rpc_main.hh"

#include "rpc_defs.hh"

#include <mpi.h>

#include <iostream>

namespace rpc {
  
  using namespace boost;
  
  using std::cerr;
  
  
  
  int initialized;
  
  
  
  void init(int& argc, char**& argv)
  {
    MPI_Initialized(&initialized);
    if (!initialized) {
      int provided;
      MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
      if (provided != MPI_THREAD_MULTIPLE) {
        cerr << "MPI does not support multi-threading\n";
        exit(1);
      }
    }
    
    comm = mpi::communicator(MPI_COMM_WORLD, mpi::comm_duplicate);
    
    // if (comm.rank() != 0) {
    //   // Don't let the non-root processes return to main
    //   event_loop();
    //   finalize();
    //   exit(0);
    // }
    
    // event_loop_thread = thread(event_loop);
  }
  
  void finalize()
  {
    // if (rank == 0) {
    //   for (int p=0; p<size(); ++p) {
    //     MPI_Send(const_cast<func_id_t*>(&id_stop), 1, get_mpi_datatype<id_t>(),
    //              p, 0, comm_);
    //   }
    // }
    // event_loop_thread.join();
    
    if (!initialized) {
      MPI_Finalize();
    }
  }
  
}



int rpc_main(int argc, char** argv);

int main(int argc, char** argv)
{
  using namespace rpc;
  rpc::init(argc, argv);
  const proc_t root = 0;
  int iret;
  if (comm.rank() == root) {
    iret = ::rpc_main(argc, argv);
  }
  mpi::broadcast(comm, iret, root);
  rpc::finalize();
  return iret;
}
