#include <cxx/task>
#include <funhpc/main>
#include <funhpc/rexec>
#include <qthread/future>
#include <qthread/mutex>
#include <qthread/thread>

// #include <mpi.h>
#include <qthread.h>

#include <vector>

namespace funhpc {

std::vector<task_t> send_queue;
qthread::mutex send_queue_mutex;

void send_task(task_t &&t) {
  qthread::lock_guard<qthread::mutex> g(send_queue_mutex);
  send_queue.push_back(std::move(t));
}

void recv_tasks() {
  std::vector<task_t> recv_queue;
  {
    qthread::lock_guard<qthread::mutex> g(send_queue_mutex);
    using std::swap;
    swap(send_queue, recv_queue);
  }
  for (auto &t : recv_queue)
    qthread::thread(std::move(t)).detach();
}

int event_loop(int &argc, char **&argv) {
  // Initialize
  // MPI_Initialize(&argc, &argv);
  qthread_initialize();
  int res;
  {
    auto fres = qthread::async(funhpc_main, argc, argv);
    while (!fres.is_ready()) {
      recv_tasks();
      qthread::this_thread::yield();
    }
    res = fres.get();
  }
  // Shut down
  qthread_finalize();
  // MPI_Finalize();
  return res;
}
}

int main(int argc, char **argv) { return funhpc::event_loop(argc, argv); }
