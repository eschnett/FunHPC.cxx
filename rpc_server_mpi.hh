#ifndef RPC_SERVER_MPI_HH
#define RPC_SERVER_MPI_HH

#include "rpc_call.hh"
#include "rpc_server.hh"

#include <boost/mpi.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

// TODO
#include <boost/archive/text_oarchive.hpp>
#include <sstream>



namespace rpc {
  
  using boost::optional;
  
  using std::async;
  using std::atomic;
  using std::min;
  using std::mutex;
  using std::lock_guard;
  using std::thread;
  using std::vector;
  
  
  
  // TODO: use invoke? make it work similar to async; improve async as
  // well so that it can call member functions.
  template<typename M, typename F, typename... As>
  auto with_lock(M& m, const F& f, const As&... args) -> decltype(f(args...))
  {
    lock_guard<decltype(m)> g(m);
    return f(args...);
  }
  
  
  
  class server_mpi;
  void terminate_stage_1();
  void terminate_stage_2();
  struct terminate_stage_1_action:
    public action_impl<terminate_stage_1_action,
                       wrap<decltype(terminate_stage_1), terminate_stage_1>>
  {
  };
  struct terminate_stage_2_action:
    public action_impl<terminate_stage_2_action,
                       wrap<decltype(terminate_stage_2), terminate_stage_2>>
  {
  };
  
  
  
  class server_mpi: public abstract_server {
    
    int& argc;
    char**& argv;
    
    boost::mpi::environment env;
    boost::mpi::communicator comm;
    
    struct send_item_t {
      int dest;
      shared_ptr<callable_base> call;
    };
    typedef vector<send_item_t> send_queue_t;
    send_queue_t send_queue;
    mutex send_queue_mutex;
    
    atomic<int> termination_stage;
    atomic<int> stage_1_counter;
    int iret;
    
  public:
    
    server_mpi(int& argc, char**& argv):
      argc(argc), argv(argv),
      env(argc, argv),
      termination_stage(0)
    {
      comm = boost::mpi::communicator(MPI_COMM_WORLD,
                                      boost::mpi::comm_duplicate);
      rank_ = comm.rank();
      size_ = comm.size();
      
      std::cout << "hardware concurrency: "
                << thread::hardware_concurrency() << "\n";
    }
    
    virtual ~server_mpi()
    {
      assert(we_should_terminate());
    }
    
    
    
  private:
    
    // Communication tree
    const int fan_out = 3;
    int child_min() const
    {
      return rank() * fan_out + 1;
    }
    int child_max() const
    {
      return min(size(), child_min() + fan_out);
    }
    int child_count() const
    {
      return child_max() - child_min();
    }
    int parent() const
    {
      if (rank() == 0) return -1;
      return (rank()-1) / fan_out;
    }
    
    // Propagate termination information
    bool we_should_terminate() const { return termination_stage == 2; }
    void terminate_stage_1()
    {
      assert(termination_stage == 0);
      termination_stage = 1;
      stage_1_counter = 0;
      for (int proc = child_min(); proc < child_max(); ++proc) {
        sync(proc, terminate_stage_1_action());
      }
      terminate_stage_2();
    }
    friend void terminate_stage_1();
    void terminate_stage_2()
    {
      assert(termination_stage == 1);
      const int value = ++stage_1_counter;
      if (value == child_count() + 1) {
        const int proc = parent();
        if (proc >= 0) {
          sync(proc, terminate_stage_2_action());
        }
        termination_stage = 2;
      }
    }
    friend void terminate_stage_2();
    
    
    
    void run_application(const user_main_t& user_main)
    {
      iret = user_main(argc, argv);
      terminate_stage_1();
    }
    
    
    
  public:
    
    virtual int event_loop(const user_main_t& user_main)
    {
      // Start main program, but only on process 0
      if (comm.rank() == 0) {
        thread([=]() { run_application(user_main); }).detach();
      }
      
      // Post first receive
      const int tag = 0;
      // TODO: send/recv several messages combined
      shared_ptr<callable_base> recv_call;
      boost::mpi::request req =
        comm.irecv(boost::mpi::any_source, tag, recv_call);
      
      bool did_recv = true, did_send = true;
      while (!we_should_terminate()) {
        // TODO: If there is nothing to do, maybe wait for some time
        // Receive
        did_recv = false;
        for (;;) {
          optional<boost::mpi::status> st = req.test();
          if (!st) break;
          did_recv = true;
          thread([=](){ (*recv_call)(); }).detach();
          // Post next receive
          recv_call = nullptr;
          req = comm.irecv(boost::mpi::any_source, tag, recv_call);
        }
        // Send
        send_queue_t my_queue;
        with_lock(send_queue_mutex, [&](){ send_queue.swap(my_queue); });
        did_send = false;
        for (const auto& send_item: my_queue) {
          did_send = true;
          // TODO: use isend
          comm.send(send_item.dest, tag, send_item.call);
        }
        // Wait
        if (!did_recv && !did_send) {
          std::this_thread::yield();
          // this_thread::sleep_for();
        }
      }
      
      // Cancel last receive
      req.cancel();
      
      // Broadcast return value
      boost::mpi::broadcast(comm, iret, 0);
      return iret;
    }
    
    
    
    virtual void call(int dest, shared_ptr<callable_base> func)
    {
      assert(!we_should_terminate());
      assert(func);
      // TODO: allow disabling this for testing
      // if (dest == rank()) {
      //   thread([=](){ (*func)(); }).detach();
      //   return;
      // }
      with_lock(send_queue_mutex,
                [&](){ send_queue.push_back(send_item_t{ dest, func }); });
    }
  };
  
  
  
  void terminate_stage_1() { ((server_mpi*)server)->terminate_stage_1(); }
  void terminate_stage_2() { ((server_mpi*)server)->terminate_stage_2(); }
  
}

BOOST_CLASS_EXPORT(rpc::terminate_stage_1_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_1_action::finish);
BOOST_CLASS_EXPORT(rpc::terminate_stage_2_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_2_action::finish);

#endif  // RPC_SERVER_MPI_HH
