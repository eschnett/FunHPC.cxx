#include <cxx/cstdlib.hpp>
#include <cxx/task.hpp>
#include <funhpc/async.hpp>
#include <funhpc/hwloc.hpp>
#include <funhpc/rexec.hpp>
#include <funhpc/server.hpp>
#include <qthread/future.hpp>
#include <qthread/mutex.hpp>
#include <qthread/thread.hpp>

#include <mpi.h>
#include <qthread.h>

#include <pthread.h>
#include <sys/time.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
// #include <stdlib.h>
#include <string>
#include <vector>

namespace funhpc {

const ptrdiff_t max_idle_count = 1000;

namespace detail {
double gettime() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec / 1.0e+6;
}

bool run_main_everywhere() {
  return cxx::envtol("FUNHPC_MAIN_EVERYWHERE", "0");
}
}

// Enable/disable communication

namespace detail {
std::unique_ptr<qthread::mutex> comm_mutex;
}
void comm_lock() {
  if (size() == 1)
    return;
  detail::comm_mutex->lock();
}
void comm_unlock() {
  if (size() == 1)
    return;
  detail::comm_mutex->unlock();
}

// Enable/disable threading
namespace detail {
bool threading_disabled = false;
pthread_rwlock_t threads_disable;
std::atomic<int> threads_counter;
}
bool threading_disabled() { return detail::threading_disabled; }
void threading_disable() {
  if (threading_disabled()) {
    std::cerr << "threading_disable: Threaading already locked\n";
    std::terminate();
  }
  detail::threading_disabled = true;
  if (qthread::thread::hardware_concurrency() == 1)
    return;
  int ierr = pthread_rwlock_init(&detail::threads_disable, NULL);
  assert(!ierr);
  ierr = pthread_rwlock_wrlock(&detail::threads_disable);
  assert(!ierr);
  detail::threads_counter = 0;
  const int numworkers = qthread::thread::hardware_concurrency() - 1;
  for (int n = 0; n < numworkers; ++n)
    qthread::async(qthread::launch::async, [&]() {
      ++detail::threads_counter;
      int ierr = pthread_rwlock_rdlock(&detail::threads_disable);
      assert(!ierr);
      ierr = pthread_rwlock_unlock(&detail::threads_disable);
      assert(!ierr);
      --detail::threads_counter;
    });
  // Note: Don't yield here, so that the master thread (which holds
  // the write lock) does not try to obtain a read lock
  // Wait until all workers are suspended
  while (detail::threads_counter != numworkers)
    ;
}
void threading_enable() {
  if (!threading_disabled()) {
    std::cerr << "threading_disable: Threaading already unlocked\n";
    std::terminate();
  }
  detail::threading_disabled = false;
  if (qthread::thread::hardware_concurrency() == 1)
    return;
  int ierr = pthread_rwlock_unlock(&detail::threads_disable);
  assert(!ierr);
  // Wait until all workers are active again
  while (detail::threads_counter != 0)
    ;
  ierr = pthread_rwlock_destroy(&detail::threads_disable);
  assert(!ierr);
}

// MPI

constexpr int mpi_root = 0;
constexpr int mpi_tag = 0;
bool did_initialize_mpi = false;
MPI_Comm mpi_comm = MPI_COMM_NULL;

namespace detail {
std::ptrdiff_t the_rank = -1;
std::ptrdiff_t the_size = -1;
std::ptrdiff_t the_local_rank = -1;
std::ptrdiff_t the_local_size = -1;
std::ptrdiff_t the_node_rank = -1;
std::ptrdiff_t the_node_size = -1;
void set_rank_size() {
  int rank, size;
  MPI_Comm_rank(mpi_comm, &rank);
  MPI_Comm_size(mpi_comm, &size);
  the_rank = rank;
  the_size = size;

  // Taken and adapted from
  // <https://github.com/jeffhammond/MPI-plus-MPI-slides/blob/master/code/hello-mpi.c>:
  MPI_Comm node_comm;
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL,
                      &node_comm);

  int local_rank, local_size;
  MPI_Comm_rank(node_comm, &local_rank);
  MPI_Comm_size(node_comm, &local_size);
  the_local_rank = local_rank;
  the_local_size = local_size;

  int node_group;
  MPI_Allreduce(&rank, &node_group, 1, MPI_INT, MPI_MIN, node_comm);
  std::vector<int> node_groups;
  if (rank == mpi_root) {
    node_groups.resize(size);
  }
  MPI_Gather(&node_group, 1, MPI_INT, node_groups.data(), 1, MPI_INT, mpi_root,
             mpi_comm);
  std::vector<int> node_ranks;
  int node_size;
  if (rank == mpi_root) {
    std::vector<int> group_ranks(size, -1);
    node_ranks.resize(size);
    node_size = 0;
    for (int p = 0; p < size; ++p) {
      int g = node_groups[p];
      int r = group_ranks[g];
      if (r < 0)
        r = group_ranks[g] = node_size++;
      node_ranks[p] = r;
    }
  }
  int node_rank;
  MPI_Scatter(node_ranks.data(), 1, MPI_INT, &node_rank, 1, MPI_INT, mpi_root,
              mpi_comm);
  MPI_Bcast(&node_size, 1, MPI_INT, mpi_root, mpi_comm);
  the_node_rank = node_rank;
  the_node_size = node_size;

  MPI_Comm_free(&node_comm);

  int num_threads = qthread::thread::hardware_concurrency();

  if (rank == mpi_root)
    std::cout << "FunHPC: Using " << node_size << " nodes, " << local_size
              << " processes per node, " << num_threads
              << " threads per process\n"
              << std::flush;

  int want_num_nodes = cxx::envtol("FUNHPC_NUM_NODES", "0");
  assert(node_size == want_num_nodes);
  // int want_node_npus = cxx::envtol("FUNHPC_NUM_PUS", "0");
  // assert(node_npus == want_node_npus);

  int want_num_procs = cxx::envtol("FUNHPC_NUM_PROCS", "0");
  assert(size == want_num_procs);
  int want_num_threads = cxx::envtol("FUNHPC_NUM_THREADS", "0");
  assert(num_threads == want_num_threads);
}
}

struct mpi_req_t {
  mpi_req_t() {}

  // MPI_Request cannot be copied or moved since MPI may keep a
  // pointer to it
  mpi_req_t(const mpi_req_t &) = delete;
  mpi_req_t(mpi_req_t &&) = delete;
  mpi_req_t &operator=(const mpi_req_t &) = delete;
  mpi_req_t &operator=(mpi_req_t &&) = delete;

  std::ptrdiff_t proc;
  std::string buf;
  MPI_Request req;
};

// Send queue, to communicate between threads
// TODO: Use Qthread's qdqueue instead?
std::vector<std::unique_ptr<mpi_req_t>> send_queue;
std::unique_ptr<qthread::mutex> send_queue_mutex;

// Send requests, to communicate with MPI
std::vector<std::unique_ptr<mpi_req_t>> send_reqs;

// Step 1: Enqueue task (from any thread)
void enqueue_task(std::ptrdiff_t dest, task_t &&t) {
  assert(size() > 1);
  if (size() == 1) {
    std::cerr << "Called enqueue_task with a single MPI process\n";
    std::terminate();
  }
  assert(dest >= 0 && dest < size());
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
bool send_tasks() {
  bool did_send = false;

  // Obtain send queue
  std::vector<std::unique_ptr<mpi_req_t>> reqps;
  {
    // TODO: Use atomic compare-and-swap, with three queues
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
    did_send = true;
  }

  // Clean up all items that are finished sending
  // TODO: Use MPI_Testsome instead
  send_reqs.erase(std::remove_if(send_reqs.begin(), send_reqs.end(),
                                 [&did_send](auto &reqp) {
                                   int flag;
                                   MPI_Test(&reqp->req, &flag,
                                            MPI_STATUS_IGNORE);
                                   did_send |= flag;
                                   return flag;
                                 }),
                  send_reqs.end());

  return did_send;
}

void cancel_sends() {
  // Is this actually necessary?
  for (auto &reqp : send_reqs)
    MPI_Cancel(&reqp->req);
  send_reqs.clear();
}

// Step 4: Run the task (in a new thread)
void run_task(std::unique_ptr<mpi_req_t> &&reqp) {
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
bool recv_tasks() {
  bool did_recv = false;
  for (;;) {
    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, mpi_tag, mpi_comm, &flag, &status);
    if (!flag)
      return did_recv;
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
    // MPI_Request req;
    // MPI_Irecv(const_cast<char *>(reqp->buf.data()), reqp->buf.size(),
    //           MPI_CHAR, reqp->proc, mpi_tag, mpi_comm, &req);
    // MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
    // if (!flag) {
    //   std::cerr << "MPI_Test not ready\n";
    //   std::terminate();
    // }
    qthread::thread(run_task, std::move(reqp)).detach();
    did_recv = true;
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
  // MPI_Init(&argc, &argv);
  // TODO: Want MPI_THREAD_FUNNELED
  int flag;
  MPI_Initialized(&flag);
  int provided;
  if (!flag) {
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    did_initialize_mpi = true;
  } else {
    MPI_Query_thread(&provided);
  }
  if (provided != MPI_THREAD_SERIALIZED && provided != MPI_THREAD_MULTIPLE) {
    std::cerr << "MPI does not support multi-threading\n";
    std::exit(EXIT_FAILURE);
  }
  MPI_Comm_dup(MPI_COMM_WORLD, &mpi_comm);
  qthread_initialize();
  detail::set_rank_size();
  hwloc_set_affinity();
  MPI_Barrier(mpi_comm);
}

int run_main(mainfunc_t *user_main, int argc, char **argv) {
  if (rank() == mpi_root) {
    auto nprocs = local_size();
    {
      std::ostringstream buf;
      buf << "FunHPC thread affinity:\n";
      for (int p = 0; p < nprocs; ++p)
        buf << funhpc::async(funhpc::rlaunch::async, p, hwloc_get_cpu_infos)
                   .get();
      std::cout << buf.str();
    }
  }

  {
    std::ostringstream buf;
    buf << "FunHPC[" << rank() << "]: begin main\n";
    std::cout << buf.str() << std::flush;
  }
  auto start_time = detail::gettime();
  int res = user_main(argc, argv);
  auto end_time = detail::gettime();
  auto run_time = end_time - start_time;
  {
    std::ostringstream buf;
    buf << "FunHPC[" << rank()
        << "]: end main; total execution time: " << run_time << " sec\n";
    std::cout << buf.str() << std::flush;
  }

  return res;
}

int eventloop(mainfunc_t *user_main, int argc, char **argv) {
  if (size() == 1)
    return run_main(user_main, argc, argv);

  detail::comm_mutex = std::make_unique<qthread::mutex>();
  send_queue_mutex = std::make_unique<qthread::mutex>();

  qthread::future<int> fres;
  if (detail::run_main_everywhere() || rank() == mpi_root)
    fres = qthread::async(run_main, user_main, argc, argv);

  std::cout << "FunHPC[" << rank() << "]: begin eventloop\n" << std::flush;
  for (;;) {
    comm_lock();
    send_tasks();
    recv_tasks();
    comm_unlock();
    if (terminate_check(!fres.valid() || fres.ready()))
      break;
    qthread::this_thread::yield();
  }
  std::cout << "FunHPC[" << rank() << "]: end eventloop\n" << std::flush;
  cancel_sends();

  send_queue_mutex.reset();
  std::cout << "FunHPC[" << rank() << "]: begin comm_mutex.reset()\n"
            << std::flush;
  detail::comm_mutex.reset();
  std::cout << "FunHPC[" << rank() << "]: end comm_mutex.reset()\n"
            << std::flush;
  return fres.valid() ? fres.get() : 0;
}

void finalize() {
  // This is not necessary
  // MPI_Barrier(mpi_comm);
  // It is recommended to only call this when necessary
  // qthread_finalize();
  if (did_initialize_mpi) {
    int flag;
    MPI_Finalized(&flag);
    if (!flag) {
      MPI_Finalize();
    }
  }
}
}
