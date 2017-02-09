#ifndef FUNHPC_SERVER_HPP
#define FUNHPC_SERVER_HPP

namespace funhpc {
typedef int mainfunc_t(int argc, char **argv);
void initialize(int &argc, char **&argv);
int eventloop(mainfunc_t *user_main, int argc, char **argv);
void finalize();
void comm_lock();
void comm_unlock();
bool threading_disabled();
void threading_disable();
void threading_enable();
}

#define FUNHPC_SERVER_HPP_DONE
#endif // #ifdef FUNHPC_SERVER_HPP
#ifndef FUNHPC_SERVER_HPP_DONE
#error "Cyclic include dependency"
#endif
