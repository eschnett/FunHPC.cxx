#include "qthread_thread.hh"

#include <algorithm>
#include <cstdlib>
#include <iostream>



namespace rpc {
  int real_main(int argc, char** argv);
}



namespace qthread {
  
  std::atomic<std::ptrdiff_t> thread::threads_started;
  std::atomic<std::ptrdiff_t> thread::threads_stopped;
  
  aligned_t thread::run_thread(void* args_)
  {
    auto args = (thread_args*)args_;
    args->func();
    args->p.set_value();
    delete args;
    // ++threads_stopped;
    return 0;
  }
  
  future<void> thread::start_thread(const std::function<void()>& func)
  // future<void> thread::start_thread(rpc::unique_function<void()>&& func)
  {
    auto args = new thread_args(func);
    auto f = args->p.get_future();
    // ++threads_started;
    int ierr = qthread_fork_syncvar(run_thread, args, NULL);
    RPC_ASSERT(!ierr);
    return f;
  }
  
  
  
  int thread_main(int argc, char** argv)
  {
    return rpc::real_main(argc, argv);
  }
  
  void thread_initialize()
  {
    qthread_initialize();
  }
  
  void thread_finalize()
  {
    qthread_finalize();
  }
  
  void thread_finalize2()
  {
  }
  
}
