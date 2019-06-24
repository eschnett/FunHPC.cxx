#include <funhpc/rexec.hpp>
#include <qthread/future.hpp>

#include <cereal/access.hpp>
#include <gtest/gtest.h>

#include <memory>

namespace {
// Cannot have global variables with qthread:: types, since Qthreads
// is initialized too late and finalized too early
std::unique_ptr<qthread::promise<void>> p;
} // namespace

void set_value() { p->set_value(); }

TEST(funhpc_rexec, local) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(0, set_value);
  p->get_future().wait();
  p.reset();
}

namespace {
void reflect() { funhpc::rexec(0, set_value); }
} // namespace

TEST(funhpc_rexec, pingpong) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(1 % funhpc::size(), reflect);
  p->get_future().wait();
  p.reset();
}

namespace {
class reflect_obj {
  // friend class cereal::access;
  // template <typename Archive> void serialize(Archive &ar) {}

public:
  void operator()() { reflect(); }
};
} // namespace

TEST(funhpc_rexec, function_object) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(1 % funhpc::size(), reflect_obj());
  p->get_future().wait();
  p.reset();
}

TEST(funhpc_rexec, member_function_pointer) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(1 % funhpc::size(), &reflect_obj::operator(), reflect_obj());
  p->get_future().wait();
  p.reset();
}

namespace {
class reflect_obj1 {
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {}

public:
  int m;
  void operator()() { reflect(); }
};
} // namespace

TEST(funhpc_rexec, function_object1) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(1 % funhpc::size(), reflect_obj1());
  p->get_future().wait();
  p.reset();
}

TEST(funhpc_rexec, member_function_pointer1) {
  p = std::make_unique<qthread::promise<void>>();
  funhpc::rexec(1 % funhpc::size(), &reflect_obj1::operator(), reflect_obj1());
  p->get_future().wait();
  p.reset();
}

namespace {
void daisy_chain(std::ptrdiff_t ttl) {
  if (ttl == 0)
    return reflect();
  funhpc::rexec((funhpc::rank() + 1) % funhpc::size(), daisy_chain, ttl - 1);
}
} // namespace

TEST(funhpc_rexec, daisy_chaining) {
  p = std::make_unique<qthread::promise<void>>();
  daisy_chain(1000);
  p->get_future().wait();
  p.reset();
}
