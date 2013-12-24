#include "qthread_thread.hh"

#include <algorithm>
#include <cstdlib>



namespace rpc {
  int real_main(int argc, char** argv);
}



namespace qthread {
  
  detached_threads* detached = nullptr;
  
  
  
  void thread::detach()
  {
    assert(mgr);
    detached->add(mgr);
    mgr = nullptr;
  }
  
  
  
  detached_threads::~detached_threads()
  {
    assert(detached.empty());
    assert(incoming.empty());
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
    for (;;) {
      bool did_some_work = false;
      
      // Empty incoming queue
      {
        auto old_size = detached.size();
        std::vector<thread_manager*> tmp;
        {
          lock_guard<mutex> g(mtx);
          swap(tmp, incoming);
        }
        detached.insert(detached.end(), tmp.begin(), tmp.end());
        did_some_work |= detached.size() != old_size;
      }
      
      // Walk through all threads, and destruct them if they are done
      {
        auto old_size = detached.size();
        auto new_end = std::remove_if(detached.begin(), detached.end(),
                                      [](thread_manager* mgr) {
                                        auto done = mgr->done();
                                        if (done) delete mgr;
                                        return done;
                                      });
        detached.erase(new_end, detached.end());
        did_some_work |= detached.size() != old_size;
      }
      
      // Wait
      // TODO: Sleep dynamically, measuring e.g. the time spent
      // checking threads
      if (!did_some_work) {
        if (signal_stop) break;
        this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    
    do {
      for (auto mgr: detached) delete mgr;
      detached.clear();
      {
        lock_guard<mutex> g(mtx);
        swap(detached, incoming);
      }
    } while (!detached.empty());
  }
  
  
  
  int thread_main(int argc, char** argv)
  {
    return rpc::real_main(argc, argv);
  }
  
  void thread_initialize()
  {
    setenv("QTHREAD_INFO", "1", 1);
    setenv("QTHREAD_STACK_SIZE", "81920", 1);
    qthread_initialize();
    detached = new detached_threads();
    detached->start_cleanup();
  }
  
  void thread_finalize()
  {
    detached->stop_cleanup();
    delete detached;
    qthread_finalize();
  }
  
  void thread_finalize2()
  {
  }
  
}
