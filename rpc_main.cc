#include "rpc_thread.hh"
#include "rpc_server.hh"
#include "rpc_server_mpi.hh"

#include <hwloc.h>

#include <mpi.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int rpc_main(int argc, char **argv);

namespace rpc {

void check_procs() {
  int nprocs = server->size();
  if (server->rank() == 0) {
    std::cout << "MPI processes: " << server->size() << "\n";
  }
  char *rpc_processes = getenv("RPC_PROCESSES");
  if (rpc_processes) {
    try {
      int rpc_nprocs = stoi(std::string(rpc_processes));
      if (rpc_nprocs != nprocs) {
        std::cout << "[" << server->rank() << "] "
                  << "WARNING: Number of MPI processes (" << nprocs
                  << ") is inconsistent with the environment variable "
                     "RPC_PROCESSES (" << rpc_nprocs << ")\n";
      }
    }
    catch (...) {
      std::cout << "[" << server->rank() << "] "
                << "WARNING: Environment variable RPC_PROCESSES ("
                << rpc_processes << ") is not an integer\n";
    }
  } else {
    std::cout << "[" << server->rank() << "] "
              << "WARNING: Environment variable RPC_PROCESSES is not set\n";
  }
}

void check_threads() {
  int nthreads = thread::hardware_concurrency();
  if (server->rank() == 0) {
    std::cout << "Hardware threads: " << nthreads << "\n";
  }
  char *rpc_threads = getenv("RPC_THREADS");
  if (rpc_threads) {
    try {
      int rpc_nthreads = stoi(std::string(rpc_threads));
      if (rpc_nthreads != nthreads) {
        std::cout
            << "[" << server->rank() << "] "
            << "WARNING: Number of hardware threads (" << nthreads
            << ") is inconsistent with the environment variable RPC_THREADS ("
            << rpc_nthreads << ")\n";
      }
    }
    catch (...) {
      std::cout << "[" << server->rank() << "] "
                << "WARNING: Environment variable RPC_THREADS (" << rpc_threads
                << ") is not an integer\n";
    }
  } else {
    std::cout << "[" << server->rank() << "] "
              << "WARNING: Environment variable RPC_THREADS is not set\n";
  }
}

void check_nodes() {
  std::vector<char> namebuf(MPI_MAX_PROCESSOR_NAME + 1);
  int length;
  MPI_Get_processor_name(namebuf.data(), &length);
  namebuf[length + 1] = '\0';
  std::vector<char> namebufs;
  if (server->rank() == 0) {
    namebufs.resize(server->size() * (MPI_MAX_PROCESSOR_NAME + 1));
  }
  MPI_Gather(namebuf.data(), MPI_MAX_PROCESSOR_NAME + 1, MPI_CHAR,
             namebufs.data(), MPI_MAX_PROCESSOR_NAME + 1, MPI_CHAR, 0,
             MPI_COMM_WORLD);
  int nnodes;
  if (server->rank() == 0) {
    std::vector<std::string> names(server->size());
    for (int p = 0; p < server->size(); ++p) {
      names.at(p) = &namebufs[p * (MPI_MAX_PROCESSOR_NAME + 1)];
    }
    std::sort(names.begin(), names.end());
    assert(!names.empty());
    nnodes = 1;
    for (int p = 1; p < server->size(); ++p) {
      nnodes += names.at(p) != names.at(p - 1);
    }
  }
  MPI_Bcast(&nnodes, 1, MPI_INT, 0, MPI_COMM_WORLD);
  if (server->rank() == 0) {
    std::cout << "Compute nodes: " << nnodes << "\n";
  }
  char *rpc_nodes = getenv("RPC_NODES");
  if (rpc_nodes) {
    try {
      int rpc_nnodes = stoi(std::string(rpc_nodes));
      if (rpc_nnodes != nnodes) {
        std::cout << "[" << server->rank() << "] "
                  << "WARNING: Number of nodes (" << nnodes
                  << ") is inconsistent with the environment variable "
                     "RPC_NODES (" << rpc_nodes << ")\n";
      }
    }
    catch (...) {
      std::cout << "[" << server->rank() << "] "
                << "WARNING: Environment variable RPC_NODES (" << rpc_nodes
                << ") is not an integer\n";
    }
  } else {
    std::cout << "[" << server->rank() << "] "
              << "WARNING: Environment variable RPC_NODES is not set\n";
  }
}

void check_cores() {
  int ierr;
  hwloc_topology_t topology;
  ierr = hwloc_topology_init(&topology);
  assert(!ierr);
  ierr = hwloc_topology_load(topology);
  assert(!ierr);
  int pu_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_PU);
  assert(pu_depth >= 0);
  int npus = hwloc_get_nbobjs_by_depth(topology, pu_depth);
  hwloc_topology_destroy(topology);
  // We ignore hyperthreads here
  int ncores = npus;
  if (server->rank() == 0) {
    std::cout << "Cores per node: " << ncores << "\n";
  }
  char *rpc_cores = getenv("RPC_CORES");
  if (rpc_cores) {
    try {
      int rpc_ncores = stoi(std::string(rpc_cores));
      if (rpc_ncores != ncores) {
        std::cout << "[" << server->rank() << "] "
                  << "WARNING: Number of cores (" << ncores
                  << ") is inconsistent with the environment variable "
                     "RPC_CORES (" << rpc_cores << ")\n";
      }
    }
    catch (...) {
      std::cout << "[" << server->rank() << "] "
                << "WARNING: Environment variable RPC_CORES (" << rpc_cores
                << ") is not an integer\n";
    }
  } else {
    std::cout << "[" << server->rank() << "] "
              << "WARNING: Environment variable RPC_CORES is not set\n";
  }
}

void check_procs_threads() {
  check_procs();
  check_threads();
  check_nodes();
  check_cores();
}

int real_main(int argc, char **argv) {
  // MPI_Init(&argc, &argv);
  rpc::server = new rpc::server_mpi(argc, argv);
  rpc::thread_initialize();
  rpc::check_procs_threads();
  if (rpc::server->rank() == 0) {
    std::cout << "Running...\n";
  }
  int iret = rpc::server->event_loop(rpc_main);
  if (rpc::server->rank() == 0) {
    if (iret == 0) {
      std::cout << "Done: success.\n";
    } else {
      std::cout << "Done: failure (error=" << iret << ").\n";
    }
  }
  rpc::thread_finalize();
  delete rpc::server;
  rpc::server = nullptr;
  // MPI_Finalize();
  return iret;
}
}

int main(int argc, char **argv) { return rpc::thread_main(argc, argv); }
