#ifndef QTHREAD_HH
#define QTHREAD_HH

#include "qthread_future.hh"
#include "qthread_mutex.hh"
#include "qthread_thread.hh"

#define QTHREAD_HH_DONE
#else
#  ifndef QTHREAD_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // QTHREAD_HH
