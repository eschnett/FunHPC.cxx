#include "hwloc.hh"
#include "rpc.hh"

#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <vector>

using rpc::async;
using rpc::client;
using rpc::future;
using rpc::launch;
using rpc::make_client;
using rpc::make_ready_future;
using rpc::make_remote_client;
using rpc::remote;
using rpc::server;
using rpc::shared_future;
using rpc::sync;

using std::cerr;
using std::cout;
using std::flush;
using std::ofstream;
using std::ios_base;
using std::make_shared;
using std::max;
using std::min;
using std::move;
using std::numeric_limits;
using std::ostream;
using std::ostringstream;
using std::ptrdiff_t;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;

////////////////////////////////////////////////////////////////////////////////

// Integer division, rounding down
template <typename T> T div_floor(T x, T y) {
  assert(y > 0);
  T r = (x >= 0 ? x : x - y + 1) / y;
  assert(r * y <= x && (r + 1) * y > x);
  return r;
}

// Integer modulo, rounding down
template <typename T> T mod_floor(T x, T y) {
  T r = x - div_floor(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

// Integer division, rounding up
template <typename T> T div_ceil(T x, T y) {
  assert(y > 0);
  T r = (x > 0 ? x + y - 1 : x) / y;
  assert(r * y >= x && (r - 1) * y < x);
  return r;
}

// Integer modulo, rounding up
template <typename T> T mod_ceil(T x, T y) {
  T r = x - div_ceil(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

////////////////////////////////////////////////////////////////////////////////

// An empty serializable type as mix-in
using empty = tuple<>;

////////////////////////////////////////////////////////////////////////////////

// Global definitions, a poor man's parameter file

namespace defs {
const ptrdiff_t rho = 100; // resolution scale

const double tmin = 0.0;
const double xmin = 0.0;
const double xmax = 1.0;

const ptrdiff_t ncells = 10 * rho;
const double dx = (xmax - xmin) / ncells;
double x(ptrdiff_t i) { return xmin + (i + 0.5) * dx; }

const double cfl = 0.5;
const double dt = cfl * dx;

const int nsteps = 2 * 100; // ncells;
const int wait_every = 0;
const int info_every = nsteps / 10;
const int file_every = 0; // nsteps;

const ptrdiff_t ncells_per_grid = 10;
}

////////////////////////////////////////////////////////////////////////////////

// A norm

struct norm_t {
  double sum, sum2, count;

private:
  friend class cereal::access;
  template <class Archive> void serialize(Archive &ar) { ar(sum, sum2, count); }

public:
  norm_t() : sum(0.0), sum2(0.0), count(0.0) {}
  norm_t(double val) : sum(val), sum2(pow(val, 2)), count(1.0) {}
  // norm_t(future<norm_t> &&x) : norm_t(move(x.get())) {}
  norm_t(const norm_t &x, const norm_t &y)
      : sum(x.sum + y.sum), sum2(x.sum2 + y.sum2), count(x.count + y.count) {}
  double avg() const { return sum / count; }
  double norm2() const { return sqrt(sum2 / count); }
};
RPC_COMPONENT(norm_t);

////////////////////////////////////////////////////////////////////////////////

// A cell -- a container holding data, no intelligence here

struct cell_t {
  static constexpr double nan = numeric_limits<double>::quiet_NaN();

  double u;
  double rho;
  double v;

private:
  friend class cereal::access;
  template <class Archive> void serialize(Archive &ar) { ar(u, rho, v); }

public:
  // For safety
  cell_t() : u(nan), rho(nan), v(nan) {}

  // Output
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    return os << "cell: u=" << u << " rho=" << rho << " v=" << v;
  }

  // Linear combination
  struct axpy : empty {};
  cell_t(axpy, double a, const cell_t &x, const cell_t &y) {
    u = a * x.u + y.u;
    rho = a * x.rho + y.rho;
    v = a * x.v + y.v;
  }

  // Norm
  norm_t norm() const {
    return norm_t(norm_t(norm_t(u), norm_t(rho)), norm_t(v));
  }

  // Analytic solution
  struct analytic : empty {};
  cell_t(analytic, double t, double x) {
    u = sin(2 * M_PI * t) * sin(2 * M_PI * x);
    rho = 2 * M_PI * cos(2 * M_PI * t) * sin(2 * M_PI * x);
    v = 2 * M_PI * sin(2 * M_PI * t) * cos(2 * M_PI * x);
  }

  // Initial condition
  struct initial : empty {};
  cell_t(initial, double t, double x) : cell_t(analytic(), t, x) {}

  // Boundary condition
  struct boundary : empty {};
  cell_t(boundary, double t, double x) : cell_t(analytic(), t, x) {}

  // Error
  struct error : empty {};
  cell_t(error, const cell_t &c, double t, double x)
      : cell_t(axpy(), -1.0, cell_t(analytic(), t, x), c) {}

  // RHS
  struct rhs : empty {};
  cell_t(rhs, const cell_t &c, const cell_t &cm, const cell_t &cp) {
    u = c.rho;
    rho = (cp.v - cm.v) / (2 * defs::dx);
    v = (cp.rho - cm.rho) / (2 * defs::dx);
  }
};
RPC_COMPONENT(cell_t);

// Output
ostream &operator<<(ostream &os, const cell_t &c) { return c.output(os); }

////////////////////////////////////////////////////////////////////////////////

// Each grid lives on a process

struct grid_t {
  double t;
  ptrdiff_t imin, imax; // spatial indices
  vector<cell_t> cells;

private:
  friend class cereal::access;
  template <class Archive> void serialize(Archive &ar) {
    ar(t, imin, imax, cells);
  }

public:
  grid_t() {} // only for serialization

private:
  grid_t(double t, ptrdiff_t imin, ptrdiff_t imax)
      : t(t), imin(imin), imax(imax), cells(imax - imin) {}
  grid_t(const client<grid_t> &g) : grid_t(g->t, g->imin, g->imax) {}
  void set(ptrdiff_t i, const cell_t &c) {
    assert(i >= imin && i < imax);
    cells[i - imin] = c;
  }
  // template <typename F> void set_cells(const F &f) {
  //   void(sizeof cell_t(f(ptrdiff_t())));
  //   for (ptrdiff_t i = imin; i < imax; ++i) {
  //     set(i, f(i));
  //   }
  // }

public:
  const cell_t &get(ptrdiff_t i) const {
    assert(i >= imin && i < imax);
    return cells[i - imin];
  }

  cell_t get_boundary(bool face_upper) const {
    return get(face_upper ? imax - 1 : imin);
  }
  RPC_DECLARE_CONST_MEMBER_ACTION(grid_t, get_boundary);

  // Output
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    os << "grid: t=" << t << "\n";
    for (ptrdiff_t i = imin; i < imax; ++i) {
      os << "   i=" << i << " x=" << defs::x(i) << " " << get(i) << "\n";
    }
    return os;
  }

  // Linear combination
  struct axpy : empty {};
  grid_t(axpy, double a, client<grid_t> x, client<grid_t> y) : grid_t(y) {
    // TODO: Call make_local on incoming clients
    // TODO: Implement futurize: convert client args to shared_ptr, then create
    // remote client
    t = a * x->t + y->t;
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::axpy(), a, x->get(i), y->get(i)));
    }
    // set_cells([&](ptrdiff_t i) {
    //   return cell_t(cell_t::axpy(), a, x->get(i), y->get(i));
    // });
  }
  RPC_DECLARE_CONSTRUCTOR(grid_t, axpy, double, client<grid_t>, client<grid_t>);

  // Norm
  norm_t norm() const {
    norm_t n;
    for (ptrdiff_t i = imin; i < imax; ++i) {
      n = norm_t(n, get(i).norm());
    }
    return n;
  }
  RPC_DECLARE_CONST_MEMBER_ACTION(grid_t, norm);

  // Initial condition
  struct initial : empty {};
  grid_t(initial, double t, ptrdiff_t imin, ptrdiff_t imax)
      : grid_t(t, imin, imax) {
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::initial(), t, defs::x(i)));
    }
  }
  RPC_DECLARE_CONSTRUCTOR(grid_t, initial, double, ptrdiff_t, ptrdiff_t);

  // Error
  struct error : empty {};
  grid_t(error, client<grid_t> g) : grid_t(g) {
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::error(), g->get(i), t, defs::x(i)));
    }
  }
  RPC_DECLARE_CONSTRUCTOR(grid_t, error, client<grid_t>);

  // RHS
  struct rhs : empty {};
  grid_t(rhs, client<grid_t> g, shared_future<cell_t> bm,
         shared_future<cell_t> bp)
      : grid_t(g) {
    t = 1.0; // dt/dt
    for (ptrdiff_t i = imin; i < imax; ++i) {
      const cell_t &c = g->get(i);
      // choose left neighbour
      ptrdiff_t im = i - 1;
      cell_t cm;
      if (im >= imin) { // same grid (interior)
        cm = g->get(im);
      } else if (im >= 0) { // other grid (ghost)
        cm = bm.get();
      } else { // boundary
        cm = cell_t(cell_t::boundary(), g->t, defs::x(im));
      }
      // choose right neighbour
      ptrdiff_t ip = i + 1;
      cell_t cp;
      if (ip < imax) { // same grid (interior)
        cp = g->get(ip);
      } else if (ip < defs::ncells) { // other grid (ghost)
        cp = bp.get();
      } else { // boundary
        cp = cell_t(cell_t::boundary(), g->t, defs::x(ip));
      }
      set(i, cell_t(cell_t::rhs(), c, cm, cp));
    }
  }
  RPC_DECLARE_CONSTRUCTOR(grid_t, rhs, client<grid_t>, shared_future<cell_t>,
                          shared_future<cell_t>);
};
RPC_COMPONENT(grid_t);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(grid_t, get_boundary);
RPC_IMPLEMENT_CONSTRUCTOR(grid_t, grid_t::axpy, double, client<grid_t>,
                          client<grid_t>);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(grid_t, norm);
RPC_IMPLEMENT_CONSTRUCTOR(grid_t, grid_t::initial, double, ptrdiff_t,
                          ptrdiff_t);
RPC_IMPLEMENT_CONSTRUCTOR(grid_t, grid_t::error, client<grid_t>);
RPC_IMPLEMENT_CONSTRUCTOR(grid_t, grid_t::rhs, client<grid_t>,
                          shared_future<cell_t>, shared_future<cell_t>);

// Output
ostream &operator<<(ostream &os, const client<grid_t> &g) {
  return g->output(os);
}

////////////////////////////////////////////////////////////////////////////////

// The domain is distributed over multiple processes

struct domain_t {
  // TODO: implement serialize routine

  double t;

  static ptrdiff_t ngrids() {
    return div_ceil(defs::ncells, defs::ncells_per_grid);
  }
  static ptrdiff_t cell2grid(ptrdiff_t icell) {
    assert(icell >= 0 && icell < defs::ncells);
    return div_floor(icell, defs::ncells_per_grid);
  }
  vector<client<grid_t> > grids;

private:
  // Note: The grids are not allocated in the constructor; this has to
  // be done explicitly.
  domain_t(double t) : t(t), grids(ngrids()) {}
  domain_t(const client<domain_t> &d) : t(d->t), grids(ngrids()) {}
  void set(ptrdiff_t i, const client<grid_t> &g) {
    assert(i >= 0 && i < ngrids() && grids.size() == ngrids());
    grids[i] = g;
  }

public:
  // Note: Be careful about this being a reference! This only works
  // because references decay in bind and async.
  const client<grid_t> &get(ptrdiff_t i) const {
    assert(i >= 0 && i < ngrids() && grids.size() == ngrids());
    return grids[i];
  }

  // // Iterator interface
  // typedef decltype(grids)::value_type value_type;
  // auto cbegin() const -> decltype(grids.cbegin()) { return grids.cbegin(); }
  // auto cend() const -> decltype(grids.cend()) { return grids.cend(); }

  // Wait until all grids are ready
  void wait() const {
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      get(i).wait();
    }
  }

  // Output
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    os << "domain: t=" << t << "\n";
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      // Serialize output by making local copies
      os << "grid[" << i << "]:\n" << get(i).make_local();
    }
    return os;
  }

  // Linear combination
  struct axpy : empty {};
  domain_t(axpy, double a, const client<domain_t> &x, const client<domain_t> &y)
      : domain_t(y) {
    t = a * x->t + y->t;
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      set(i,
          make_remote_client<grid_t>(y->get(i).get_proc_future(),
                                     grid_t::axpy(), a, x->get(i), y->get(i)));
    }
  }

  // Norm
  norm_t norm() const {
    // TODO: Use a proper reduction operation
    vector<future<norm_t> > fns;
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      fns.push_back(async(remote::async, grid_t::norm_action(), get(i)));
    }
    norm_t n;
    for (auto &fn : fns)
      n = norm_t(n, fn.get());
    return n;
  }
  // static norm_t norm(const client<domain_t> &d) {
  //   struct reduce {
  //     norm_t operator()(const norm_t &a, const norm_t &b) const {
  //       return norm_t(a, b);
  //     }
  //   };
  //   struct zero {
  //     norm_t operator()() const { return norm_t(); }
  //   };
  //   return map_reduce(grid_t::norm_action(), reduce(), zero(), d);
  // }

  // Initial condition
  struct initial : empty {};
  domain_t(initial, double t) : domain_t(t) {
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      // Choose a domain decomposition
      int proc = mod_floor(rpc::server->rank() + int(i), rpc::server->size());
      ptrdiff_t imin = i * defs::ncells_per_grid;
      ptrdiff_t imax = min((i + 1) * defs::ncells_per_grid, defs::ncells);
      set(i,
          make_remote_client<grid_t>(proc, grid_t::initial(), t, imin, imax));
    }
  }

  // Error
  struct error : empty {};
  domain_t(error, const client<domain_t> &s) : domain_t(s) {
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      const client<grid_t> &si = s->get(i);
      set(i, make_remote_client<grid_t>(si.get_proc_future(), grid_t::error(),
                                        si));
    }
  }

  // RHS
  struct rhs : empty {};
  domain_t(rhs, const client<domain_t> &s) : domain_t(s) {
    // TODO: handle outer boundary conditions as well
    t = 1.0; // dt/dt
    const shared_future<cell_t> fake = make_ready_future(cell_t());
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      const client<grid_t> &si = s->get(i);
      // left buondary
      ptrdiff_t im = i - 1;
      shared_future<cell_t> cm;
      if (im >= 0) {
        const client<grid_t> &sim = s->get(im);
        cm = async(remote::async, grid_t::get_boundary_action(), sim, true);
      } else {
        cm = fake;
      }
      // right boundary
      ptrdiff_t ip = i + 1;
      shared_future<cell_t> cp;
      if (ip < ngrids()) {
        const client<grid_t> &sip = s->get(ip);
        cp = async(remote::async, grid_t::get_boundary_action(), sip, false);
      } else {
        cp = fake;
      }
      set(i, make_remote_client<grid_t>(si.get_proc_future(), grid_t::rhs(), si,
                                        cm, cp));
    }
  }
};

// Output
ostream &operator<<(ostream &os, const client<domain_t> &d) {
  return d->output(os);
}

////////////////////////////////////////////////////////////////////////////////

// Memoized data for the current iteration

struct memoized_t {
  ptrdiff_t n;
  client<domain_t> state;
  client<domain_t> rhs;
  client<domain_t> error;
  shared_future<norm_t> error_norm;
  memoized_t(ptrdiff_t n, const client<domain_t> &state) : n(n), state(state) {
    rhs = make_client<domain_t>(launch::deferred, domain_t::rhs(), state);
    error = make_client<domain_t>(launch::deferred, domain_t::error(), state);
    error_norm = async(launch::deferred, &domain_t::norm, error);
  }
};

// RK2
client<domain_t> rk2(const shared_ptr<memoized_t> &m) {
  const client<domain_t> &s0 = m->state;
  const client<domain_t> &r0 = m->rhs;
  // Step 1
  auto s1 = make_client<domain_t>(domain_t::axpy(), 0.5 * defs::dt, r0, s0);
  auto r1 = make_client<domain_t>(domain_t::rhs(), s1);
  // Step 2
  return make_client<domain_t>(domain_t::axpy(), defs::dt, r1, s0);
}

// Output
ostream *do_info_output(const shared_future<ostream *> &fos,
                        const shared_ptr<memoized_t> &m) {
  RPC_ASSERT(server->rank() == 0);
  const shared_ptr<domain_t> &s = m->state.get();
  const norm_t &en = m->error_norm.get();
  ostream *os = fos.get();
  *os << "n=" << m->n << " t=" << s->t << " "
      << "L2-norm[error]: " << en.norm2() << "\n" << flush;
  return os;
}

shared_future<ostream *> info_output(shared_future<ostream *> fos,
                                     const shared_ptr<memoized_t> &m) {
  if (defs::info_every != 0 &&
      (m->n == defs::nsteps || m->n % defs::info_every == 0)) {
    fos = async(do_info_output, fos, m);
  }
  return fos;
}

ostream *do_file_output(const shared_future<ostream *> fos,
                        const shared_ptr<memoized_t> &m) {
  RPC_ASSERT(server->rank() == 0);
  ostream *os = fos.get();
  *os << "State: " << m->state << "RHS: " << m->rhs
      << "L2-norm[error]: " << m->error_norm.get().norm2() << "\n"
      << "\n" << std::flush;
  return os;
}

shared_future<ostream *> file_output(shared_future<ostream *> fos,
                                     const shared_ptr<memoized_t> &m) {
  if (defs::file_every != 0 &&
      (m->n == defs::nsteps || m->n % defs::file_every == 0)) {
    fos = async(do_file_output, fos, m);
  }
  return fos;
}

////////////////////////////////////////////////////////////////////////////////

// Driver

string get_thread_stats() {
  ostringstream os;
  rpc::thread_stats_t thread_stats = rpc::get_thread_stats();
  os << "[" << rpc::server->rank() << "] Thread statistics:\n"
     << "   threads started: " << thread_stats.threads_started << "\n"
     << "   threads stopped: " << thread_stats.threads_stopped << "\n"
     << "   threads still running: "
     << thread_stats.threads_started - thread_stats.threads_stopped << "\n";
  return os.str();
}
RPC_ACTION(get_thread_stats);

string get_server_stats() {
  ostringstream os;
  rpc::server_base::stats_t server_stats = rpc::server->get_stats();
  os << "[" << rpc::server->rank() << "] Server statistics:\n"
     << "   messages sent: " << server_stats.messages_sent << "\n"
     << "   messages received: " << server_stats.messages_received << "\n";
  return os.str();
}
RPC_ACTION(get_server_stats);

int rpc_main(int argc, char **argv) {
  cout << "Determining CPU bindings via hwloc:\n";
  hwloc_bindings(false);
  // cout << "Setting CPU bindings via hwloc:\n";
  // hwloc_bindings(true);

  shared_future<ostream *> fio = make_ready_future<ostream *>(&cout);
  ostringstream filename;
  filename << "wave." << rpc::server->size() << ".txt";
  ofstream file(filename.str(), ios_base::trunc);
  shared_future<ostream *> ffo = make_ready_future<ostream *>(&file);

  auto t0 = std::chrono::high_resolution_clock::now();

  auto s = make_client<domain_t>(domain_t::initial(), defs::tmin);
  auto m = make_shared<memoized_t>(0, s);
  fio = info_output(fio, m);
  ffo = file_output(ffo, m);

  while (m->n < defs::nsteps) {

    if (defs::wait_every != 0 && m->n > 0 && m->n % defs::wait_every == 0) {
      // Rate limiter
      m->state.wait();
    }

    s = rk2(m);
    m = make_shared<memoized_t>(m->n + 1, s);
    fio = info_output(fio, m);
    ffo = file_output(ffo, m);
  }

  fio.wait();
  ffo.wait();

  auto t1 = std::chrono::high_resolution_clock::now();
  double elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
      1.0e+9;

  cout << "Elapsed time: " << elapsed << " sec\n";
  for (int p = 0; p < rpc::server->size(); ++p) {
    cout << sync(remote::sync, p, get_thread_stats_action());
  }
  for (int p = 0; p < rpc::server->size(); ++p) {
    cout << sync(remote::sync, p, get_server_stats_action());
  }

  file.close();

  return 0;
}
