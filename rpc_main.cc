#include "rpc_main.hh"

#include "rpc_defs.hh"

#include <mpi.h>

#include <boost/optional.hpp>

#include <algorithm>
#include <future>
#include <iostream>
#include <thread>



namespace rpc {
  
  using namespace boost;
  
  using boost::optional;
  
  using std::cerr;
  using std::cout;
  using std::flush;
  using std::min;
  using std::shared_future;
  using std::thread;
  
  
  
  mpi::environment* env = nullptr;
  
  
  
  void send_termination_signal()
  {
    // // Propagate termination in a tree
    // const int fan_out = 2;
    // const int min_dest = fan_out * (comm.rank()+1) - 1;
    // const int max_dest = min(comm.size(), min_dest + fan_out);
    // assert(min_dest > comm.rank());
    // for (int dest = min_dest; dest < max_dest; ++dest) {
    //   comm.send(dest, tag, id_stop);
    // }
  }
  
  void terminate(int iret)
  {
    delete env;
    env = nullptr;
    exit(iret);
  }
  
  const int tag = 0;
  typedef int func_id_t;
  const func_id_t id_stop = 0;
  void event_loop()
  {
    for (;;) {
      // Receive
      for (;;) {
        func_id_t id;
        mpi::request req = comm.irecv(mpi::any_source, tag, id);
        optional<mpi::status> st = req.test();
        if (!st) break;
        if (id == id_stop) terminate(0);
        int source = st->source();
        // const any_action_t* func = actions().lookup(id);
        // assert(func);
        // async(func);
      }
      // Send
      // while (!sendqueue.empty()) {
      //   ...
      // }
    }
  }
  
  void run_application(int (&user_main)(int argc, char** argv),
                        int argc, char** argv)
  {
    int iret = user_main(argc, argv);
    cout << "[main()=" << iret << "]\n";
    // TODO: pass iret
    // for (int dest=0; dest<comm.size(); ++dest) comm.send(1, tag, id_stop);
    mpi::environment::abort(iret);
  }
  
  void init(int (&user_main)(int argc, char** argv),
            int argc, char** argv)
  {
    env = new mpi::environment(argc, argv);
    comm = mpi::communicator(MPI_COMM_WORLD, mpi::comm_duplicate);
    cout << "hardware concurrency: " << thread::hardware_concurrency() << "\n";
    
    if (comm.rank() == 0) {
      // std::async(run_application, user_main, argc, argv);
      std::async([=]() { return run_application(user_main, argc, argv); });
    }
    
    event_loop();
  }
  
}



int rpc_main(int argc, char** argv);

int main(int argc, char** argv)
{
  rpc::init(rpc_main, argc, argv);
  return 1;                     // unreachable
}
