#ifndef FUNHPC_REXEC_HPP
#define FUNHPC_REXEC_HPP

#include <cxx/task.hpp>
#include <qthread/thread.hpp>

#include <type_traits>

namespace funhpc {
namespace detail {
extern std::ptrdiff_t the_rank;
extern std::ptrdiff_t the_size;
extern std::ptrdiff_t the_local_rank;
extern std::ptrdiff_t the_local_size;
extern std::ptrdiff_t the_node_rank;
extern std::ptrdiff_t the_node_size;
}
// Processes
inline std::ptrdiff_t rank() { return detail::the_rank; }
inline std::ptrdiff_t size() { return detail::the_size; }
// Node-local processes
inline std::ptrdiff_t local_rank() { return detail::the_local_rank; }
inline std::ptrdiff_t local_size() { return detail::the_local_size; }
// Nodes
inline std::ptrdiff_t node_rank() { return detail::the_node_rank; }
inline std::ptrdiff_t node_size() { return detail::the_node_size; }

typedef cxx::task<void> task_t;
void enqueue_task(std::ptrdiff_t dest, task_t &&t);

// Remote execution
template <typename F, typename... Args>
void rexec(std::ptrdiff_t dest, F &&f, Args &&... args) {
  if (dest == rank())
    return qthread::thread(std::forward<F>(f), std::forward<Args>(args)...)
        .detach();
  task_t::register_type<std::decay_t<F>, std::decay_t<Args>...>();
  enqueue_task(dest, task_t(std::forward<F>(f), std::forward<Args>(args)...));
}
}

#define FUNHPC_REXEC_HPP_DONE
#endif // #ifdef FUNHPC_REXEC_HPP
#ifndef FUNHPC_REXEC_HPP_DONE
#error "Cyclic include dependency"
#endif
