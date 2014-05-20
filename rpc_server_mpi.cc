#include "rpc_server_mpi.hh"

namespace rpc {
  
  server_mpi::server_mpi(int& argc, char**& argv):
    argc(argc), argv(argv),
    env(argc, argv),
    termination_stage(0)
  {
    comm = boost::mpi::communicator(MPI_COMM_WORLD, boost::mpi::comm_duplicate);
    rank_ = comm.rank();
    size_ = comm.size();
  }
  
  server_mpi::~server_mpi()
  {
    RPC_ASSERT(we_should_terminate());
  }
  
  
  
  void server_mpi::terminate_stage_1()
  {
    RPC_ASSERT(termination_stage == 0);
    termination_stage = 1;
    stage_1_counter = 0;
    for (int proc = child_min(); proc < child_max(); ++proc) {
      detached(remote::detached, proc, terminate_stage_1_action());
    }
    terminate_stage_2();
  }
  
  void server_mpi::terminate_stage_2()
  {
    RPC_ASSERT(termination_stage == 1);
    const int value = ++stage_1_counter;
    if (value == child_count() + 1) {
      const int proc = parent();
      if (proc >= 0) {
        detached(remote::detached, proc, terminate_stage_2_action());
      }
      termination_stage = 2;
      if (proc < 0) {
        detached(remote::detached, 0, terminate_stage_3_action());
        }
    }
  }
  
  void server_mpi::terminate_stage_3()
  {
    RPC_ASSERT(termination_stage == 2);
    termination_stage = 3;
    stage_3_counter = 0;
    for (int proc = child_min(); proc < child_max(); ++proc) {
      detached(remote::detached, proc, terminate_stage_3_action());
    }
    terminate_stage_4();
  }
  
  void server_mpi::terminate_stage_4()
  {
    RPC_ASSERT(termination_stage == 3);
    const int value = ++stage_3_counter;
    if (value == child_count() + 1) {
      const int proc = parent();
      if (proc >= 0) {
        detached(remote::detached, proc, terminate_stage_4_action());
      }
      termination_stage = 4;
    }
  }
  
  
  
  int server_mpi::event_loop(const user_main_t& user_main)
  {
#ifndef RPC_DISABLE_CALL_SHORTCUT
    if (comm.size() == 1) {
      // Optimization: Don't start the MPI communication server
      iret = user_main(argc, argv);
      termination_stage = 4;
      boost::mpi::broadcast(comm, iret, 0);
      return iret;
    }
#endif
    
    // Start main program, but only on process 0
    if (comm.rank() == 0) {
      thread([=]() {
          iret = user_main(argc, argv);
          terminate_stage_1();
        }).detach();
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
    vector<shared_ptr<callable_base> > recv_calls(num_recvs);
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
                           vector<boost::mpi::request>::iterator> > st =
          boost::mpi::test_any(recv_reqs.begin(), recv_reqs.end());
        if (!st) break;
        did_communicate = true;
        auto& recv_req = *st->second;
        const int i = st->second - recv_reqs.begin();
        auto& recv_call = recv_calls[i];
        if (! (we_should_stop_sending() &&
               (typeid(*recv_call) !=
                typeid(rpc::terminate_stage_3_action::evaluate)) &&
               (typeid(*recv_call) !=
                typeid(rpc::terminate_stage_4_action::evaluate))))
        {
          // TODO: move recv_call instead of copying it
          thread(&callable_base::execute, recv_call).detach();
        }
        recv_call = nullptr;
        // Post next receive
        recv_req = comm.irecv(source(i), tag, recv_call);
      }
      // Finalize sends
      for (;;) {
        optional<std::pair<boost::mpi::status,
                           list<boost::mpi::request>::iterator> > st =
          boost::mpi::test_any(send_reqs.begin(), send_reqs.end());
        if (!st) break;
        did_communicate = true;
        send_reqs.erase(st->second);
      }
      // Wait
      if (!did_communicate) {
        this_thread::yield();
        // this_thread::sleep_for(std::chrono::microseconds(1));
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
  
  
  
  void server_mpi::call(int dest, shared_ptr<callable_base> func)
  {
    RPC_ASSERT(dest>=0 && dest<size());
    RPC_ASSERT(func!=nullptr);
#ifndef RPC_DISABLE_CALL_SHORTCUT
    RPC_ASSERT(dest != rank());
#endif
    // Threads may still be active when we need to terminate; let
    // them enqueue requests (why not?)
    if (we_should_stop_sending() &&
        typeid(*func) != typeid(rpc::terminate_stage_3_action::evaluate) &&
        typeid(*func) != typeid(rpc::terminate_stage_4_action::evaluate))
    {
      // // TODO: block thread instead of sleeping
      // nthis_thread::sleep_for(std::chrono::seconds(1000000));
      // RPC_ASSERT(0);
      // This assumes that the calling thread will not attempt to
      // perform significant work
      return;
    }
    // RPC_ASSERT(!we_should_terminate());
    // Enable this output to debug unregistered and unexported classes
#ifdef RPC_DEBUG_MISSING_EXPORTS
    rpc::with_lock(rpc::io_mutex, [&]{
        std::cout << "[" << rpc::server->rank() << "] "
                  << "sending type " << typeid(*func).name() << " "
                  << "to " << dest << "\n";
      });
#endif
    // TODO: use atomic swaps instead of a mutex
    with_lock(send_queue_mutex,
              [&]{ send_queue.push_back(send_item_t{ dest, func }); });
  }
  
  
  
  void terminate_stage_1()
  {
    ((server_mpi*)server)->terminate_stage_1();
  }
  void terminate_stage_2()
  {
    ((server_mpi*)server)->terminate_stage_2();
  }
  void terminate_stage_3()
  {
    ((server_mpi*)server)->terminate_stage_3();
  }
  void terminate_stage_4()
  {
    ((server_mpi*)server)->terminate_stage_4();
  }
  
}

RPC_IMPLEMENT_ACTION(rpc::terminate_stage_1);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_2);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_3);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_4);
