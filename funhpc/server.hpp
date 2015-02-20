// -*-C++-*-
#ifndef FUNHPC_SERVER_HPP
#define FUNHPC_SERVER_HPP

namespace funhpc {
typedef int mainfunc_t(int argc, char **argv);
void initialize(int &argc, char **&argv);
int eventloop(mainfunc_t *user_main, int argc, char **argv);
void finalize();
}

#define FUNHPC_SERVER_HPP_DONE
#endif // #ifdef FUNHPC_SERVER_HPP
#ifndef FUNHPC_SERVER_HPP_DONE
#error "Cyclic include dependency"
#endif
