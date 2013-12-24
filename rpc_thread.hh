#ifndef RPC_THREAD_HH
#define RPC_THREAD_HH

// Note: HPX requires that this header is the first header to be
// included, so that an HPX header is included before any boost
// headers.

// Choose threading implementation:
//    STL:      works everywhere, may have bad performance
//    qthreads: portable and efficient library, but the C++ wrapper is
//              not tuned for performance yet
//    HPX:      good performance, but difficult to build

#if defined RPC_STL
#  include "stl_thread.hh"
#elif defined RPC_QTHREADS
#  include "qthread.hh"
#elif defined RPC_HPX
#  include "hpx.hh"
#else
#  error "No threading library specified"
#endif

// Initialization sequence:
//    main
//       rpc::thread_main [provided by threading library]
//          rpc::real_main
//             MPI server construct
//             thread_initialize
//             rpc_main [application]
//             thread_finalize
//             MPI server destruct
//             thread_finalize2

#define RPC_THREAD_HH_DONE
#else
#  ifndef RPC_THREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // RPC_THREAD_HH
