#ifndef RPC_SERVER_HH
#define RPC_SERVER_HH

#include <functional>

namespace rpc {
  
  struct callable_base;
  
  struct abstract_server {
    typedef std::function<int(int argc, char** argv)> user_main_t;
    
    virtual ~abstract_server() {}
    
    virtual int event_loop(const user_main_t& user_main) = 0;
    
    virtual int rank() const = 0;
    virtual int size() const = 0;
    virtual void call(callable_base& func, int dest) = 0;
  };
  
  extern abstract_server* server;
  
}

#endif  // RPC_SERVER_HH
