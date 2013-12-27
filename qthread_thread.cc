#include "qthread_thread.hh"

#include <algorithm>
#include <cstdlib>



namespace rpc {
  int real_main(int argc, char** argv);
}



namespace qthread {
  
  
  aligned_t thread::run_thread(void* args_)
  {
    auto args = (thread_args*)args_;
    args->func();
    args->p.set_value();
    delete args;
    return 0;
  }
  
  future<void> thread::start_thread(const std::function<void()>& func)
  {
    auto args = new thread_args(func);
    auto f = args->p.get_future();
    int ierr = qthread_fork_syncvar(run_thread, args, NULL);
    assert(!ierr);
    return f;
  }
  
  
  
  int thread_main(int argc, char** argv)
  {
    return rpc::real_main(argc, argv);
  }
  
  void thread_initialize()
  {
    setenv("QTHREAD_INFO", "1", 1);
    setenv("QTHREAD_STACK_SIZE", "131071", 1);
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
