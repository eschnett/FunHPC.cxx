#ifndef RPC_SERVER_MPI_HH
#define RPC_SERVER_MPI_HH

#include "rpc_action.hh"
#include "rpc_call.hh"
#include "rpc_defs.hh"
#include "rpc_server.hh"

#include <mpi.h>

namespace rpc {

class server_mpi;
void terminate_stage_1();
void terminate_stage_2();
void terminate_stage_3();
void terminate_stage_4();
RPC_DECLARE_ACTION(terminate_stage_1);
RPC_DECLARE_ACTION(terminate_stage_2);
RPC_DECLARE_ACTION(terminate_stage_3);
RPC_DECLARE_ACTION(terminate_stage_4);

class server_mpi : public server_base {

  int &argc;
  char **&argv;

  MPI_Comm comm;

  struct send_item_t {
    int dest;
    std::shared_ptr<callable_base> call;
  };
  typedef std::vector<send_item_t> send_queue_t;
  send_queue_t send_queue;
  mutex send_queue_mutex;

  std::atomic<int> termination_stage;
  std::atomic<int> stage_1_counter;
  std::atomic<int> stage_3_counter;
  int iret;

  stats_t stats;

public:
  server_mpi(int &argc, char **&argv);
  virtual ~server_mpi();

  virtual void barrier() { MPI_Barrier(comm); }

  virtual stats_t get_stats() const { return stats; }

private:
  // Communication tree
  const int fan_out = 3;
  int child_min() const { return rank() * fan_out + 1; }
  int child_max() const { return std::min(size(), child_min() + fan_out); }
  int child_count() const { return std::max(0, child_max() - child_min()); }
  int parent() const {
    if (rank() == 0)
      return -1;
    return (rank() - 1) / fan_out;
  }

  // Propagate termination information
  bool we_should_stop_sending() const { return termination_stage >= 2; }
  bool we_should_terminate() const { return termination_stage >= 4; }
  void terminate_stage_1();
  friend void terminate_stage_1();
  void terminate_stage_2();
  friend void terminate_stage_2();
  void terminate_stage_3();
  friend void terminate_stage_3();
  void terminate_stage_4();
  friend void terminate_stage_4();
  bool we_should_ignore_call(const std::shared_ptr<callable_base> &call) {
    return we_should_stop_sending() &&
           typeid(*call) != typeid(rpc::terminate_stage_3_action::evaluate) &&
           typeid(*call) != typeid(rpc::terminate_stage_4_action::evaluate);
  }

  void run_application(const user_main_t &user_main);

public:
  virtual int event_loop(const user_main_t &user_main);
  virtual void call(int dest, const std::shared_ptr<callable_base> &func);
};
}

#define RPC_SERVER_MPI_HH_DONE
#else
#ifndef RPC_SERVER_MPI_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_SERVER_MPI_HH
