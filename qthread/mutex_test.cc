#include <qthread/mutex>

using namespace qthread;

int main_mutex(int argc, char **argv) {
  mutex m;
  m.lock();
  m.unlock();

  { lock_guard<mutex> g(m); }

  return 0;
}
