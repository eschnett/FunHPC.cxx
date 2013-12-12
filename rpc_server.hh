#ifndef RPC_SERVER_HH
#define RPC_SERVER_HH

#include <boost/shared_ptr.hpp>

#include <functional>

namespace rpc {
  
  using std::function;
  


  struct callable_base;
  
  class abstract_server {
    
  protected:
    int rank_, size_;
    
  public:
    abstract_server(): rank_(-1), size_(-1) {}
    
    int rank() const { return rank_; }
    int size() const { return size_; }
    
    typedef function<int(int argc, char** argv)> user_main_t;
    
    virtual ~abstract_server() {}
    
    virtual int event_loop(const user_main_t& user_main) = 0;
    
    virtual void call(int dest, boost::shared_ptr<callable_base> func) = 0;
  };
  
  extern abstract_server* server;
  
}

#define RPC_SERVER_HH_DONE
#else
#  ifndef RPC_SERVER_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_SERVER_HH
