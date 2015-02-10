#include <cxx/task>
#include <funhpc/server>
#include <funhpc/rexec>
#include <qthread/future>
#include <qthread/mutex>
#include <qthread/thread>

#include <mpi.h>
#include <qthread.h>

#include <algorithm>
#include <memory>
#include <sstream>
// #include <stdlib.h>
#include <string>
#include <vector>

namespace funhpc {

constexpr int mpi_root = 0;
constexpr int mpi_tag = 0;
const MPI_Comm mpi_comm = MPI_COMM_WORLD;

std::size_t rank() {
  int rank;
  MPI_Comm_rank(mpi_comm, &rank);
  return rank;
}

std::size_t size() {
  int size;
  MPI_Comm_size(mpi_comm, &size);
  return size;
}

struct mpi_req_t {
  mpi_req_t() {}

  // MPI_Request cannot be copied or moved since MPI may keep a
  // pointer to it
  mpi_req_t(const mpi_req_t &) = delete;
  mpi_req_t(mpi_req_t &&) = delete;
  mpi_req_t &operator=(const mpi_req_t &) = delete;
  mpi_req_t &operator=(mpi_req_t &&) = delete;

  std::size_t proc;
  std::string buf;
  MPI_Request req;
};

// Send queue, to communicate between threads
std::vector<std::unique_ptr<mpi_req_t>> send_queue;
std::unique_ptr<qthread::mutex> send_queue_mutex;

// Send requests, to communicate with MPI
std::vector<std::unique_ptr<mpi_req_t>> send_reqs;

// Step 1: Enqueue task (from any thread)
void enqueue_task(std::size_t dest, task_t &&t) {
  // Serialize task
  auto reqp = std::make_unique<mpi_req_t>();
  reqp->proc = dest;
  std::stringstream buf;
  { (cereal::BinaryOutputArchive(buf))(std::move(t)); }
  reqp->buf = buf.str();
  {
    qthread::lock_guard<qthread::mutex> g(*send_queue_mutex);
    send_queue.push_back(std::move(reqp));
  }
}

// Step 2: Send task via MPI (from MPI thread)
void send_tasks() {
  // Obtain send queue
  std::vector<std::unique_ptr<mpi_req_t>> reqps;
  {
    qthread::lock_guard<qthread::mutex> g(*send_queue_mutex);
    using std::swap;
    swap(send_queue, reqps);
  }

  // Begin sending all queued items
  for (auto &reqp : reqps) {
    // const_cast is necessary because of an MPI API bug
    MPI_Isend(const_cast<char *>(reqp->buf.data()), reqp->buf.size(), MPI_CHAR,
              reqp->proc, mpi_tag, mpi_comm, &reqp->req);
    send_reqs.push_back(std::move(reqp));
  }

  // Clean up all items that are finished sending
  send_reqs.erase(
      std::remove_if(send_reqs.begin(), send_reqs.end(), [](auto &reqp) {
        int flag;
        MPI_Test(&reqp->req, &flag, MPI_STATUS_IGNORE);
        return flag;
      }),
      send_reqs.end());
}

void cancel_sends() {
  // Is this actually necessary?
  for (auto &reqp : send_reqs)
    MPI_Cancel(&reqp->req);
  send_reqs.clear();
}

// Step 4: Run the task (in a new thread)
void run_task(std::unique_ptr<mpi_req_t> &reqp) {
  // Deserialize task
  task_t t;
  {
    std::stringstream buf(std::move(reqp->buf));
    (cereal::BinaryInputArchive(buf))(t);
  }
  reqp.reset(); // free memory

  // Run task
  t();
}

// Step 3: Receive the task via MPI (in MPI thread)
void recv_tasks() {
  for (;;) {
    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, mpi_tag, mpi_comm, &flag, &status);
    if (!flag)
      return;
    auto reqp = std::make_unique<mpi_req_t>();
    reqp->proc = status.MPI_SOURCE;
    int count;
    MPI_Get_count(&status, MPI_CHAR, &count);
    reqp->buf.resize(count);
    // We assume the message is immediately available
    // Note: The const_cast here is against the C++ standard for
    // std::string, but works fine in practice
    MPI_Recv(const_cast<char *>(reqp->buf.data()), reqp->buf.size(), MPI_CHAR,
             reqp->proc, mpi_tag, mpi_comm, MPI_STATUS_IGNORE);
    qthread::thread(run_task, std::move(reqp)).detach();
  }
}

// In the beginning, no process is terminating. Once a process becomes
// ready to terminate, it begins terminating, and enters the barrier.
// Only after all processes have entered the barrier, we actually
// terminate.
bool terminating = false;
MPI_Request terminate_req;

bool terminate_check(bool ready_to_terminate) {
  if (!terminating) {
    if (!ready_to_terminate)
      return false;
    terminating = true;
    MPI_Ibarrier(mpi_comm, &terminate_req);
  }
  int flag;
  MPI_Test(&terminate_req, &flag, MPI_STATUS_IGNORE);
  return flag;
}

void initialize(int &argc, char **&argv) {
  MPI_Init(&argc, &argv);
  // ::setenv("QTHREAD_STACK_SIZE", "65536", 0);
  qthread_initialize();
}

int eventloop(mainfunc_t *user_main, int argc, char **argv) {
  send_queue_mutex = std::make_unique<qthread::mutex>();

  qthread::future<int> fres;
  if (rank() == mpi_root)
    fres = qthread::async(user_main, argc, argv);

  for (;;) {
    send_tasks();
    recv_tasks();
    if (terminate_check(!fres.valid() || fres.is_ready()))
      break;
    qthread::this_thread::yield();
  }
  cancel_sends();

  send_queue_mutex.reset();
  return fres.valid() ? fres.get() : 0;
}

void finalize() {
  qthread_finalize();
  MPI_Finalize();
}
}
