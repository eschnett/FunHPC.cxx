#include "qthread_thread.hh"

#include <algorithm>

namespace qthread {
  
  using std::remove_if;
  
  
  
  detached_threads* detached = nullptr;
  
  
  
  void thread::detach()
  {
    assert(mgr);
    detached->add(mgr);
    mgr = nullptr;
  }
  
  
  
  detached_threads::~detached_threads()
  {
    assert(!cleanup_thread.joinable());
  }
  
  void detached_threads::start_cleanup()
  {
    // Start cleanup thread
    signal_stop = false;
    cleanup_thread = thread([=](){ cleanup(); });
  }
  
  void detached_threads::stop_cleanup()
  {
    signal_stop = true;
    cleanup_thread.join();
  }
  
  void detached_threads::cleanup()
  {
    while (!(detached.empty() && signal_stop)) {
      // Wait
      // TODO: Sleep dynamically, measuring e.g. the time spent
      // checking threads
      this_thread::sleep_for(std::chrono::milliseconds(100));
      
      // Empty incoming queue
      vector<thread_manager*> tmp;
      {
        lock_guard<mutex> g(mtx);
        swap(tmp, incoming);
      }
      detached.insert(detached.end(), tmp.begin(), tmp.end());
      
      // Walk through all threads, and destruct them if they are done
      remove_if(detached.begin(), detached.end(),
                [](thread_manager* mgr) { return mgr->done(); });
    }
  }
  
  
  
  void initialize()
    {
    qthread_initialize();
    detached = new detached_threads();
    detached->start_cleanup();
  }
  
  void finalize()
  {
    detached->stop_cleanup();
    delete detached;
    qthread_finalize();
  }
  
}
