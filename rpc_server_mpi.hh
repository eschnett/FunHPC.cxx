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
#include <list>
#include <thread>
#include <vector>



namespace rpc {
  
  using boost::optional;
  
  using std::async;
  using std::atomic;
  using std::max;
  using std::min;
  using std::list;
  using std::thread;
  using std::vector;
  
  
  
  class server_mpi;
  void terminate_stage_1();
  void terminate_stage_2();
  void terminate_stage_3();
  void terminate_stage_4();
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
  struct terminate_stage_3_action:
    public action_impl<terminate_stage_3_action,
                       wrap<decltype(terminate_stage_3), terminate_stage_3>>
  {
  };
  struct terminate_stage_4_action:
    public action_impl<terminate_stage_4_action,
                       wrap<decltype(terminate_stage_4), terminate_stage_4>>
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
    atomic<int> stage_3_counter;
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
      
      std::cout << "[" << rank() << "] Hardware concurrency: "
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
      return max(0, child_max() - child_min());
    }
    int parent() const
    {
      if (rank() == 0) return -1;
      return (rank()-1) / fan_out;
    }
    
    // Propagate termination information
    bool we_should_stop_sending() const { return termination_stage >= 2; }
    bool we_should_terminate() const { return termination_stage >= 4; }
    void terminate_stage_1()
    {
      assert(termination_stage == 0);
      termination_stage = 1;
      stage_1_counter = 0;
      for (int proc = child_min(); proc < child_max(); ++proc) {
        apply(proc, terminate_stage_1_action());
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
          apply(proc, terminate_stage_2_action());
        }
        termination_stage = 2;
        if (proc < 0) {
          apply(0, terminate_stage_3_action());
        }
      }
    }
    friend void terminate_stage_2();
    void terminate_stage_3()
    {
      assert(termination_stage == 2);
      termination_stage = 3;
      stage_3_counter = 0;
      for (int proc = child_min(); proc < child_max(); ++proc) {
        apply(proc, terminate_stage_3_action());
      }
      terminate_stage_4();
    }
    friend void terminate_stage_3();
    void terminate_stage_4()
    {
      assert(termination_stage == 3);
      const int value = ++stage_3_counter;
      if (value == child_count() + 1) {
        const int proc = parent();
        if (proc >= 0) {
          apply(proc, terminate_stage_4_action());
        }
        termination_stage = 4;
      }
    }
    friend void terminate_stage_4();
    
    
    
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
        thread([=]{ run_application(user_main); }).detach();
      }
      
      const int num_recvs = 1;
      // const int num_recvs = server->size();
      const auto source = [](int i){ return boost::mpi::any_source; };
      // const auto source = [](int i){ return i; };
      const int tag = 0;
      
      // Post receives
      // Note: Can't have multiple receives open with any_source,
      // since Boost may break MPI messages into two that then don't
      // match any more!
      vector<shared_ptr<callable_base>> recv_calls(num_recvs);
      vector<boost::mpi::request> recv_reqs(num_recvs);
      for (int i=0; i<num_recvs; ++i) {
        recv_reqs[i] = comm.irecv(source(i), tag, recv_calls[i]);
      }
      list<boost::mpi::request> send_reqs;
      
      bool did_communicate = true;
      while (!(we_should_terminate() &&
               with_lock(send_queue_mutex, [&]{ return send_queue.empty(); })))
      {
        did_communicate = false;
        // Send
        send_queue_t my_queue;
        with_lock(send_queue_mutex, [&]{ send_queue.swap(my_queue); });
        for (const auto& send_item: my_queue) {
          did_communicate = true;
          send_reqs.push_back(comm.isend(send_item.dest, tag, send_item.call));
        }
        // Receive
        for (;;) {
          // Note: In this mpi::test call, the object is deserialized,
          // which calls the load function, which triggers an
          // mpi::send, which doesn't happen immediately since the
          // event loop is still trapped in mpi::test.
          optional<std::pair<boost::mpi::status,
                             vector<boost::mpi::request>::iterator>> st =
            boost::mpi::test_any(recv_reqs.begin(), recv_reqs.end());
          if (!st) break;
          did_communicate = true;
          auto& recv_req = *st->second;
          const int i = st->second - recv_reqs.begin();
          auto& recv_call = recv_calls[i];
          if (! (we_should_stop_sending() &&
                 typeid(*recv_call) != typeid(rpc::terminate_stage_3_action::evaluate) &&
                 typeid(*recv_call) != typeid(rpc::terminate_stage_4_action::evaluate)))
          {
            thread([=]{ (*recv_call)(); }).detach();
          }
          recv_call.reset();
          // Post next receive
          recv_req = comm.irecv(source(i), tag, recv_call);
        }
        // Finalize sends
        for (;;) {
          optional<std::pair<boost::mpi::status,
                             list<boost::mpi::request>::iterator>> st =
            boost::mpi::test_any(send_reqs.begin(), send_reqs.end());
          if (!st) break;
          did_communicate = true;
          send_reqs.erase(st->second);
        }
        // Wait
        if (!did_communicate) {
          std::this_thread::yield();
        }
      }
      
      // Cancel receives
      for (auto& recv_req: recv_reqs) recv_req.cancel();
      // Cancel sends
      for (auto& send_req: send_reqs) send_req.cancel();
      
      // Broadcast return value
      boost::mpi::broadcast(comm, iret, 0);
      return iret;
    }
    
    
    
    virtual void call(int dest, shared_ptr<callable_base> func)
    {
      assert(func);
#ifndef RPC_DISABLE_CALL_SHORTCUT
      assert(dest != rank());
#endif
      // Threads may still be active when we need to terminate; let
      // the enqueue requests (why not?)
      if (we_should_stop_sending() &&
          typeid(*func) != typeid(rpc::terminate_stage_3_action::evaluate) &&
          typeid(*func) != typeid(rpc::terminate_stage_4_action::evaluate))
      {
        // TODO: block thread instead of sleeping
        std::this_thread::sleep_for(std::chrono::seconds(1000000));
        assert(0);
      }
      // assert(!we_should_terminate());
      // TODO: use atomic swaps instead of a mutex
      with_lock(send_queue_mutex,
                [&]{ send_queue.push_back(send_item_t{ dest, func }); });
    }
  };
  
  
  
  inline void terminate_stage_1()
  {
    ((server_mpi*)server)->terminate_stage_1();
  }
  inline void terminate_stage_2()
  {
    ((server_mpi*)server)->terminate_stage_2();
  }
  inline void terminate_stage_3()
  {
    ((server_mpi*)server)->terminate_stage_3();
  }
  inline void terminate_stage_4()
  {
    ((server_mpi*)server)->terminate_stage_4();
  }
  
}

BOOST_CLASS_EXPORT(rpc::terminate_stage_1_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_1_action::finish);
BOOST_CLASS_EXPORT(rpc::terminate_stage_2_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_2_action::finish);
BOOST_CLASS_EXPORT(rpc::terminate_stage_3_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_3_action::finish);
BOOST_CLASS_EXPORT(rpc::terminate_stage_4_action::evaluate);
BOOST_CLASS_EXPORT(rpc::terminate_stage_4_action::finish);

#endif  // RPC_SERVER_MPI_HH
