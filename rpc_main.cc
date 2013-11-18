#include "rpc_main.hh"

#include "rpc_defs.hh"

#include <mpi.h>

#include <algorithm>
#include <future>
#include <iostream>
#include <thread>



namespace rpc {
  
  using namespace boost;
  
  using std::cerr;
  using std::cout;
  using std::flush;
  using std::min;
  using std::shared_future;
  using std::thread;
  
  
  
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
    cout << "hello\n" << flush;
    
    comm = mpi::communicator(MPI_COMM_WORLD, mpi::comm_duplicate);
  }
  
  void finalize()
  {
    if (!initialized) {
      MPI_Finalize();
    }
  }
  
  const int tag = 0;
  typedef int func_id_t;
  const func_id_t id_stop = 0;
  void event_loop()
  {
    for (;;) {
      func_id_t id;
      mpi::status st = comm.recv(mpi::any_source, tag, id);
      if (id == id_stop) break;
      int source = st.source();
      // const any_action_t* func = actions().lookup(id);
      // assert(func);
      // async(func);
    }
    
    // Propagate termination in a tree
    const int fan_out = 2;
    const int min_dest = fan_out * (comm.rank()+1) - 1;
    const int max_dest = min(comm.size(), min_dest + fan_out);
    assert(min_dest > comm.rank());
    for (int dest = min_dest; dest < max_dest; ++dest) {
      comm.send(dest, tag, id_stop);
    }
  }
  
  int call_user_main(int (&user_main)(int argc, char** argv),
                     int argc, char** argv)
  {
    int iret = user_main(argc, argv);
    comm.send(0, tag, id_stop);
    return iret;
  }
  
  int main1(int (&user_main)(int argc, char** argv),
            int argc, char** argv)
  {
    init(argc, argv);
    
    cout << "hardware concurrency: " << thread::hardware_concurrency() << "\n";
    
    const proc_t root = 0;
    shared_future<int> iretf;
    if (comm.rank() == root) {
      cout << "[root 0]\n" << flush;
      // iretf = std::async(call_user_main, user_main, argc, argv);
      iretf =
        std::async([=]() { return call_user_main(user_main, argc, argv); });
      cout << "[root 1]\n" << flush;
    }
    
    cout << "[event 0]\n" << flush;
    event_loop();
    cout << "[event 1]\n" << flush;
    
    int iret;
    if (comm.rank() == root) {
      cout << "[root 0]\n" << flush;
      iret = iretf.get();
      cout << "[root 1]\n" << flush;
    }
    cout << "[broadcast 0]\n" << flush;
    mpi::broadcast(comm, iret, root);
    cout << "[broadcast 1]\n" << flush;
    
    finalize();
    return iret;
  }
  
}



int rpc_main(int argc, char** argv);

int main(int argc, char** argv)
{
  return rpc::main1(rpc_main, argc, argv);
}
