#include "rpc_server_mpi.hh"

#include "cxx_utils.hh"

#include <cereal/archives/binary.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace rpc {

// Note: The small message optimisation is currently disabled, as it may not
// yield any benefit
const int max_small_size = 0;
enum tags { tag_small = 1, tag_large = 2 };

template <typename T> class send_req_t {
  std::string buf;
  MPI_Request req;
  bool req_complete;

  static void pack(std::string &buf, const T &obj) {
    std::ostringstream ss;
    {
      cereal::BinaryOutputArchive oa(ss);
      oa(obj);
    }
    buf = std::move(ss).str();
  }

public:
  // Send requests cannot be copied or moved, since the buffer must
  // remain untouched while MPI is using it
  send_req_t() = delete;
  send_req_t(const send_req_t &) = delete;
  send_req_t(send_req_t &&) = delete;
  send_req_t &operator=(const send_req_t &) = delete;
  send_req_t &operator=(send_req_t &&) = delete;
  send_req_t(const MPI_Comm &comm, int dest, const T &obj) {
    pack(buf, obj);
    int tag = buf.size() <= max_small_size ? tag_small : tag_large;
    MPI_Isend(buf.data(), buf.size(), MPI_CHAR, dest, tag, comm, &req);
    req_complete = false;
  }
  ~send_req_t() {
    if (!req_complete)
      MPI_Cancel(&req);
  }
  bool test() {
    int flag;
    MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
    return req_complete = flag;
  }
  void cancel() {
    assert(!req_complete);
    MPI_Cancel(&req);
    req_complete = true;
  }
};

template <typename T> class recv_req_t {
  MPI_Comm comm;
  std::vector<char> buf;
  MPI_Request req;
  bool req_complete;

  void post_irecv() {
    assert(req_complete);
    buf.resize(max_small_size);
    MPI_Irecv(const_cast<char *>(buf.data()), buf.size(), MPI_CHAR,
              MPI_ANY_SOURCE, tag_small, comm, &req);
    req_complete = false;
  }
  static void unpack(T &obj, std::string &&buf) {
    std::stringstream ss(std::move(buf));
    cereal::BinaryInputArchive ia(ss);
    ia(obj);
  }
  static void unpack(T &obj, std::vector<char> &&buf) {
    std::stringstream ss(std::string(buf.begin(), buf.end()));
    cereal::BinaryInputArchive ia(ss);
    ia(obj);
  }

public:
  // Receive requests cannot be copied or moved, since the buffer must
  // remain untouched while MPI is using it
  recv_req_t() = delete;
  recv_req_t(const recv_req_t &) = delete;
  recv_req_t(recv_req_t &&) = delete;
  recv_req_t &operator=(const recv_req_t &) = delete;
  recv_req_t &operator=(recv_req_t &&) = delete;
  recv_req_t(const MPI_Comm &comm) : comm(comm), req_complete(true) {
    post_irecv();
  }
  ~recv_req_t() {
    if (!req_complete)
      MPI_Cancel(&req);
  }
  template <typename F> bool test(const F &func) {
    // Check for a small message
    int recvd;
    MPI_Status st;
    MPI_Test(&req, &recvd, &st);
    if (recvd) {
      req_complete = true;
      T obj;
      unpack(obj, std::move(buf));
      post_irecv();
      func(std::move(obj));
      return true;
    }
    // Check for a large message
    MPI_Iprobe(MPI_ANY_SOURCE, tag_large, comm, &recvd, &st);
    if (recvd) {
      int sz;
      MPI_Get_count(&st, MPI_CHAR, &sz);
      std::string lbuf;
      lbuf.resize(sz);
      MPI_Recv(const_cast<char *>(lbuf.data()), lbuf.size(), MPI_CHAR,
               st.MPI_SOURCE, st.MPI_TAG, comm, MPI_STATUS_IGNORE);
      T obj;
      unpack(obj, std::move(lbuf));
      func(std::move(obj));
      return true;
    }
    return false;
  }
  void cancel() {
    assert(!req_complete);
    MPI_Cancel(&req);
    req_complete = true;
  }
};

server_mpi::server_mpi(int &argc, char **&argv)
    : argc(argc), argv(argv), termination_stage(0), stats({ 0, 0 }) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
  assert(provided != MPI_THREAD_SINGLE);
  MPI_Comm_dup(MPI_COMM_WORLD, &comm);
  MPI_Comm_rank(comm, &rank_);
  MPI_Comm_size(comm, &size_);
  // Ensure communication is working
  MPI_Barrier(comm);
}

server_mpi::~server_mpi() {
  RPC_ASSERT(we_should_terminate());
  MPI_Finalize();
}

void server_mpi::terminate_stage_1() {
  RPC_ASSERT(termination_stage == 0);
  termination_stage = 1;
  stage_1_counter = 0;
  for (int proc = child_min(); proc < child_max(); ++proc) {
    detached(rlaunch::detached, proc, terminate_stage_1_action());
  }
  terminate_stage_2();
}

void server_mpi::terminate_stage_2() {
  RPC_ASSERT(termination_stage == 1);
  const int value = ++stage_1_counter;
  if (value == child_count() + 1) {
    const int proc = parent();
    if (proc >= 0) {
      detached(rlaunch::detached, proc, terminate_stage_2_action());
    }
    termination_stage = 2;
    if (proc < 0) {
      detached(rlaunch::detached, 0, terminate_stage_3_action());
    }
  }
}

void server_mpi::terminate_stage_3() {
  RPC_ASSERT(termination_stage == 2);
  termination_stage = 3;
  stage_3_counter = 0;
  for (int proc = child_min(); proc < child_max(); ++proc) {
    detached(rlaunch::detached, proc, terminate_stage_3_action());
  }
  terminate_stage_4();
}

void server_mpi::terminate_stage_4() {
  RPC_ASSERT(termination_stage == 3);
  const int value = ++stage_3_counter;
  if (value == child_count() + 1) {
    const int proc = parent();
    if (proc >= 0) {
      detached(rlaunch::detached, proc, terminate_stage_4_action());
    }
    termination_stage = 4;
  }
}

int server_mpi::event_loop(const user_main_t &user_main) {
#ifndef RPC_DISABLE_CALL_SHORTCUT
  if (size() == 1) {
    // Optimization: Don't start the MPI communication server
    iret = user_main(argc, argv);
    termination_stage = 4;
    MPI_Bcast(&iret, 1, MPI_INT, 0, comm);
  }
#endif

  // Start main program, but only on process 0
  if (rank() == 0) {
    thread([=]() {
             iret = user_main(argc, argv);
             terminate_stage_1();
           }).detach();
  }

  // Pending requests
  typedef std::shared_ptr<callable_base> call_t;
  recv_req_t<call_t> recv_req(comm);
  std::vector<std::unique_ptr<send_req_t<call_t> > > send_reqs;

  bool did_communicate = true;
  while (!(we_should_terminate() &&
           with_lock(send_queue_mutex, [&] { return send_queue.empty(); }))) {
    did_communicate = false;
    {
      // Send all pending items
      send_queue_t my_queue;
      with_lock(send_queue_mutex, [&] { std::swap(send_queue, my_queue); });
      did_communicate |= !my_queue.empty();
      for (auto &send_item : my_queue) {
        send_reqs.push_back(cxx::make_unique<send_req_t<call_t> >(
            comm, send_item.dest, std::move(send_item.call)));
        ++stats.messages_sent;
      }
    }
    // Receive as many items as possible
    for (;;) {
      // Note: In this MPI_Test call, the object is deserialized,
      // which calls the load function, which triggers an mpi::send,
      // which doesn't happen immediately since the event loop is
      // still trapped in MPI_Test.
      bool did_receive = recv_req.test([this](const call_t &call) {
        if (!we_should_ignore_call(call))
          thread(&callable_base::execute, call).detach();
      });
      if (!did_receive)
        break;
      did_communicate = true;
      ++stats.messages_received;
    }
    // Finalize all completed sends
    {
      std::size_t old_size = send_reqs.size();
      send_reqs.erase(
          std::remove_if(std::begin(send_reqs), std::end(send_reqs),
                         [](const std::unique_ptr<send_req_t<call_t> > &call) {
            return call->test();
          }),
          std::end(send_reqs));
      did_communicate |= send_reqs.size() != old_size;
    }
    // Wait
    if (!did_communicate) {
      this_thread::yield();
      // this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

  // Cancel receives and sends
  recv_req.cancel();
  for (auto &send_req : send_reqs)
    send_req->cancel();

  // Broadcast return value
  MPI_Bcast(&iret, 1, MPI_INT, 0, comm);
  return iret;
}

void server_mpi::call(int dest, const std::shared_ptr<callable_base> &func) {
  RPC_ASSERT(dest >= 0 && dest < size());
  RPC_ASSERT(func != nullptr);
#ifndef RPC_DISABLE_CALL_SHORTCUT
  RPC_ASSERT(dest != rank());
#endif
  // Threads may still be active when we need to terminate; let
  // them enqueue requests (why not?)
  if (we_should_ignore_call(func)) {
    // // TODO: block thread instead of sleeping
    // this_thread::sleep_for(std::chrono::seconds(1000000));
    // RPC_ASSERT(0);
    // This assumes that the calling thread will not attempt to
    // perform significant work
    return;
  }
  // RPC_ASSERT(!we_should_terminate());
  // TODO: use atomic swaps instead of a mutex
  //*TODO*/send_req_t<std::shared_ptr<callable_base> >(comm, dest, func);
  with_lock(send_queue_mutex, [&] { send_queue.push_back({ dest, func }); });
}

void terminate_stage_1() { ((server_mpi *)server)->terminate_stage_1(); }
void terminate_stage_2() { ((server_mpi *)server)->terminate_stage_2(); }
void terminate_stage_3() { ((server_mpi *)server)->terminate_stage_3(); }
void terminate_stage_4() { ((server_mpi *)server)->terminate_stage_4(); }
}
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_1);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_2);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_3);
RPC_IMPLEMENT_ACTION(rpc::terminate_stage_4);
