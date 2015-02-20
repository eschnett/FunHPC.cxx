// -*-C++-*-
#ifndef QTHREAD_HPP_THREAD_HPP
#define QTHREAD_HPP_THREAD_HPP

#include <qthread/future.hpp>

#include <qthread/qthread.hpp>
#include <qthread/qt_syscalls.h>

#include <chrono>
#include <tuple>
#include <type_traits>

namespace qthread {

// thread //////////////////////////////////////////////////////////////////////

typedef detail::async_thread<void> thread;

// this_thread /////////////////////////////////////////////////////////////////

namespace this_thread {
inline void yield() { qthread_yield(); }

template <typename Rep, typename Period>
void sleep_for(const std::chrono::duration<Rep, Period> &duration) {
  auto usecs = std::chrono::microseconds(duration).count();
  timeval timeout;
  timeout.tv_sec = usecs / 1000000;
  timeout.tv_usec = usecs % 1000000;
  qt_select(0, nullptr, nullptr, nullptr, &timeout);
}
}
}

#define QTHREAD_HPP_THREAD_HPP_DONE
#endif // #ifndef QTHREAD_HPP_THREAD_HPP
#ifndef QTHREAD_HPP_THREAD_HPP_DONE
#error "Cyclic include dependency"
#endif
