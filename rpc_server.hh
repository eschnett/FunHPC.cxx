#ifndef RPC_SERVER_HH
#define RPC_SERVER_HH

#include <functional>
#include <memory>

namespace rpc {

struct callable_base;

class server_base {

protected:
  int rank_, size_;

public:
  server_base() : rank_(-1), size_(-1) {}
  virtual ~server_base() {}

  int rank() const { return rank_; }
  int size() const { return size_; }

  virtual void barrier() = 0;

  struct stats_t {
    std::ptrdiff_t messages_sent;
    std::ptrdiff_t messages_received;
  };
  virtual stats_t get_stats() const = 0;

  typedef std::function<int(int argc, char **argv)> user_main_t;

  virtual int event_loop(const user_main_t &user_main) = 0;

  virtual void call(int dest, const std::shared_ptr<callable_base> &func) = 0;
};

extern server_base *server;
}

#define RPC_SERVER_HH_DONE
#else
#ifndef RPC_SERVER_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // RPC_SERVER_HH
