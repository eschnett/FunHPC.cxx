#ifndef QTHREAD_FUTURE_FWD_HH
#define QTHREAD_FUTURE_FWD_HH

namespace qthread {

template <typename T> class future;
template <typename T> class shared_future;
template <typename T> class promise;
template <typename T> class deferred;
}

#define QTHREAD_FUTURE_FWD_HH_DONE
#else
#ifndef QTHREAD_FUTURE_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // QTHREAD_FUTURE_FWD_HH
