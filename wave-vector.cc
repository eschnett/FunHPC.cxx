// The scalar wave equation, without higher-order functions. This uses
// a vector instead of a tree to store the remote futures to the
// grids, and is thus not scalable.

#include "rpc.hh"

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using cxx::div_ceil;
using cxx::div_floor;
using cxx::mod_floor;

using rpc::async;
using rpc::broadcast;
using rpc::client;
using rpc::find_all_processes;
using rpc::future;
using rpc::launch;
using rpc::make_client;
using rpc::make_ready_future;
using rpc::make_remote_client;
using rpc::rlaunch;
using rpc::server;
using rpc::shared_future;
using rpc::sync;
using rpc::thread;

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

// Global definitions, a poor man's parameter file

struct defs_t {
#if 1                        // benchmark
  const ptrdiff_t rho = 100; // resolution scale
  const ptrdiff_t ncells_per_grid = 10;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.5;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = 10;
  const ptrdiff_t wait_every = 0;
  const ptrdiff_t info_every = 0;
  const ptrdiff_t file_every = -1;
#elif 0 // test
  const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t ncells_per_grid = 10;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.5;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = -1;
  const ptrdiff_t wait_every = 10;
  const ptrdiff_t info_every = 10;
  const ptrdiff_t file_every = 0;
#elif 0 // debug
  const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t ncells_per_grid = 10;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.5;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = -1;
  const ptrdiff_t wait_every = 1;
  const ptrdiff_t info_every = 1;
  const ptrdiff_t file_every = 1;
#else
#error "No parameter settings selected"
#endif

  ptrdiff_t ncells;
  double dx;
  double dt;

  defs_t(int nprocs, int nthreads)
      : ncells(rho * ncells_per_grid * nprocs * nthreads),
        dx((xmax - xmin) / ncells), dt(cfl * dx) {}

private:
  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) {
    ar(ncells, dx, dt);
  }

public:
  defs_t() {} // only for serialize
};
shared_ptr<defs_t> defs;
auto set_defs(shared_ptr<defs_t> defs) { ::defs = defs; }
RPC_ACTION(set_defs);
auto set_all_defs(const shared_ptr<defs_t> &defs) {
  async_broadcast(set_defs_action(), defs).get();
}

auto do_this_time(ptrdiff_t iteration, ptrdiff_t do_every) {
  if (do_every < 0)
    return false;
  if (iteration == 0)
    return true;
  if (iteration == defs->nsteps)
    return true;
  if (do_every > 0 && iteration % do_every == 0)
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////

// Linear combinations

// TODO: introduce the notion of a vector space for this?

// inline double axpy(double a, double x, double y) { return a * x + y; }
// RPC_ACTION(axpy);

////////////////////////////////////////////////////////////////////////////////

// A norm

// TODO: introduce cxx_monoid for this?
// mempty, mappend, mconcat
// also: sum, product, any, all

struct norm_t {
  double sum, sum2, count;

private:
  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) {
    ar(sum, sum2, count);
  }

public:
  norm_t() : sum(0.0), sum2(0.0), count(0.0) {}
  norm_t(double val) : sum(val), sum2(pow(val, 2)), count(1.0) {}
  norm_t(const norm_t &x, const norm_t &y)
      : sum(x.sum + y.sum), sum2(x.sum2 + y.sum2), count(x.count + y.count) {}
  auto avg() const { return sum / count; }
  auto norm2() const { return sqrt(sum2 / count); }
};
RPC_COMPONENT(norm_t);
inline auto operator+(const norm_t &x, const norm_t &y) { return norm_t(x, y); }

auto norm_mempty() { return norm_t(); }
RPC_ACTION(norm_mempty);
auto norm_mappend(const norm_t &x, const norm_t &y) { return norm_t(x, y); }
RPC_ACTION(norm_mappend);

////////////////////////////////////////////////////////////////////////////////

// A cell -- a container holding data, no intelligence here

struct cell_t {
  static constexpr auto nan = numeric_limits<double>::quiet_NaN();

  double x;
  double u;
  double rho;
  double v;

private:
  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) { ar(x, u, rho, v); }

public:
  // For safety
  cell_t() : x(nan), u(nan), rho(nan), v(nan) {}

  // Output
  auto output() const {
    ostringstream os;
    os << "cell: x=" << x << " u=" << u << " rho=" << rho << " v=" << v;
    return os.str();
  }

  // Linear combination
  struct axpy : tuple<> {};
  cell_t(axpy, double a, const cell_t &x, const cell_t &y) {
    this->x = a * x.x + y.x;
    u = a * x.u + y.u;
    rho = a * x.rho + y.rho;
    v = a * x.v + y.v;
  }

  // Norm
  auto norm() const { return norm_t(u) + norm_t(rho) + norm_t(v); }

  // Analytic solution
  struct analytic : tuple<> {};
  cell_t(analytic, double t, double x) {
    this->x = x;
    u = sin(2 * M_PI * t) * sin(2 * M_PI * x);
    rho = 2 * M_PI * cos(2 * M_PI * t) * sin(2 * M_PI * x);
    v = 2 * M_PI * sin(2 * M_PI * t) * cos(2 * M_PI * x);
  }

  // Initial condition
  struct initial : tuple<> {};
  cell_t(initial, double t, double x) : cell_t(analytic(), t, x) {}

  // Boundary condition
  struct boundary : tuple<> {};
  cell_t(boundary, double t, double x) : cell_t(analytic(), t, x) {}
  RPC_DECLARE_CONSTRUCTOR(cell_t, boundary, double, double);

  // Error
  struct error : tuple<> {};
  cell_t(error, const cell_t &c, double t)
      : cell_t(axpy(), -1.0, cell_t(analytic(), t, c.x), c) {}

  // RHS
  struct rhs : tuple<> {};
  cell_t(rhs, const cell_t &c, const cell_t &cm, const cell_t &cp) {
    x = 0.0; // dx/dt
    u = c.rho;
    rho = (cp.v - cm.v) / (2 * defs->dx);
    v = (cp.rho - cm.rho) / (2 * defs->dx);
  }
};
RPC_COMPONENT(cell_t);
RPC_IMPLEMENT_CONSTRUCTOR(cell_t, cell_t::boundary, double, double);

// Output
auto operator<<(ostream &os, const cell_t &c) -> ostream & {
  return os << c.output();
}

////////////////////////////////////////////////////////////////////////////////

// Each grid lives on a process

struct grid_t {
  // TODO: introduce irange for these two (e.g. irange_t =
  // array<ptrdiff_t,2>)
  ptrdiff_t imin, imax; // spatial indices
  static auto x(ptrdiff_t i) { return defs->xmin + (i + 0.5) * defs->dx; }

  vector<cell_t> cells;

private:
  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) {
    ar(imin, imax, cells);
  }

public:
  grid_t() {} // only for serialization

  auto get(ptrdiff_t i) const -> const cell_t & {
    assert(i >= imin && i < imax);
    return cells[i - imin];
  }

  auto get_boundary(bool face_upper) const {
    return get(face_upper ? imax - 1 : imin);
  }

  // Wait until the grid is ready
  auto wait() const {}

  // Output
  auto output() const {
    ostringstream os;
    for (ptrdiff_t i = imin; i < imax; ++i) {
      os << "   i=" << i << " " << get(i) << "\n";
    }
    return os.str();
  }

  // Linear combination
  struct axpy : tuple<> {};
  grid_t(axpy, double a, const grid_t &x, const grid_t &y)
      : imin(y.imin), imax(y.imax), cells(x.cells.size()) {
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::axpy(), a, x.cells[i], y.cells[i]);
  }

  // Norm
  auto norm() const {
    norm_t n;
    for (size_t i = 0; i < cells.size(); ++i)
      n = n + cells[i].norm();
    return n;
  }

  // Initial condition
  struct initial : tuple<> {};
  grid_t(initial, double t, ptrdiff_t imin)
      : imin(imin), imax(min(imin + defs->ncells_per_grid, defs->ncells)),
        cells(imax - imin) {
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::initial(), t, x(imin + i));
  }

  // Error
  struct error : tuple<> {};
  grid_t(error, const grid_t &g, double t)
      : imin(g.imin), imax(g.imax), cells(g.cells.size()) {
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::error(), g.cells[i], t);
  }

  // RHS
  struct rhs : tuple<> {};
  grid_t(rhs, const grid_t &g, const cell_t &bm, const cell_t &bp)
      : imin(g.imin), imax(g.imax), cells(g.cells.size()) {
    size_t s = cells.size();
    assert(s >= 1);
    if (s == 1) {
      cells[0] = cell_t(cell_t::rhs(), g.cells[0], bm, bp);
    } else {
      cells[0] = cell_t(cell_t::rhs(), g.cells[0], bm, g.cells[1]);
      for (size_t i = 1; i < s - 1; ++i)
        cells[i] =
            cell_t(cell_t::rhs(), g.cells[i], g.cells[i - 1], g.cells[i + 1]);
      cells[s - 1] = cell_t(cell_t::rhs(), g.cells[s - 1], g.cells[s - 2], bp);
    }
  }
};
RPC_COMPONENT(grid_t);

// Output
auto operator<<(ostream &os, const client<grid_t> &g) -> ostream & {
  return os << g.make_local()->output();
}

auto grid_get_boundary(const client<grid_t> &g, bool face_upper) {
  return make_client<cell_t>(g->get_boundary(face_upper));
}
RPC_ACTION(grid_get_boundary);

auto grid_wait(const client<grid_t> &g) { return g->wait(); }
RPC_ACTION(grid_wait);

auto grid_output(const client<grid_t> &g) { return g->output(); }
RPC_ACTION(grid_output);

// Note: Arguments re-ordered
auto grid_axpy(const client<grid_t> &x, double a, const client<grid_t> &y) {
  return make_client<grid_t>(grid_t::axpy(), a, *x, *y);
}
RPC_ACTION(grid_axpy);

auto grid_norm(const client<grid_t> &g) { return g->norm(); }
RPC_ACTION(grid_norm);

// Note: Arguments re-ordered
auto grid_initial(ptrdiff_t imin, double t) {
  return make_client<grid_t>(grid_t::initial(), t, imin);
}
RPC_ACTION(grid_initial);

auto grid_error(const client<grid_t> &g, double t) {
  return make_client<grid_t>(grid_t::error(), *g, t);
}
RPC_ACTION(grid_error);

auto grid_rhs(const client<grid_t> &g, const client<cell_t> &bm,
              const client<cell_t> &bp) {
  return make_client<grid_t>(grid_t::rhs(), *g, *bm, *bp);
}
RPC_ACTION(grid_rhs);

////////////////////////////////////////////////////////////////////////////////

// The domain is distributed over multiple processes

RPC_COMPONENT(vector<client<grid_t> >);

struct domain_t {
  double t;

  static auto ngrids() -> ptrdiff_t {
    return div_ceil(defs->ncells, defs->ncells_per_grid);
  }
  static auto cell2grid(ptrdiff_t icell) -> ptrdiff_t {
    assert(icell >= 0 && icell < defs->ncells);
    return div_floor(icell, defs->ncells_per_grid);
  }
  vector<client<grid_t> > grids;

  // private:
  //   friend class cereal::access;
  //   template <typename Archive> auto serialize(Archive &ar) { ar(t, grids); }

  // public:
  //   domain_t() {} // only for serialization

  // Wait until all grids are ready
  auto wait() const {
    vector<future<void> > fs;
    for (const auto &g : grids)
      fs.push_back(async(rlaunch::async, grid_wait_action(), g));
    for (auto &f : fs)
      f.wait();
  }

  auto output() const {
    ostringstream os;
    os << "domain: t=" << t << "\n";
    vector<future<string> > fs;
    for (const auto &g : grids)
      fs.push_back(async(rlaunch::async, grid_output_action(), g));
    for (auto &f : fs)
      os << f.get();
    return os.str();
  }

  // Linear combination
  struct axpy : tuple<> {};
  domain_t(axpy, double a, const domain_t &x, const domain_t &y)
      : t(a * x.t + y.t), grids(x.grids.size()) {
    for (size_t i = 0; i < grids.size(); ++i)
      grids[i] = client<grid_t>(
          async(rlaunch::async, grid_axpy_action(), x.grids[i], a, y.grids[i]));
  }
  domain_t(axpy, double a, const client<domain_t> &x, const client<domain_t> &y)
      : domain_t(axpy(), a, *x, *y) {}

  // Norm
  auto norm() const {
    vector<future<norm_t> > fs;
    for (const auto &g : grids)
      fs.push_back(async(grid_norm_action(), g));
    norm_t n;
    for (auto &f : fs)
      n = n + f.get();
    return n;
  }

  // Initial condition
  // (Also choose a domain decomposition)
  struct initial : tuple<> {};
  domain_t(initial, double t)
      : t(t), grids(div_ceil(defs->ncells, defs->ncells_per_grid)) {
    for (size_t i = 0; i < grids.size(); ++i) {
      ptrdiff_t proc = mod_floor(ptrdiff_t(i), ptrdiff_t(server->size()));
      grids[i] =
          client<grid_t>(async(rlaunch::async, proc, grid_initial_action(),
                               i * defs->ncells_per_grid, t));
    }
  }

  // Error
  struct error : tuple<> {};
  domain_t(error, const domain_t &s) : t(s.t), grids(s.grids.size()) {
    for (size_t i = 0; i < grids.size(); ++i)
      grids[i] = client<grid_t>(
          async(rlaunch::async, grid_error_action(), s.grids[i], t));
  }
  domain_t(error, const client<domain_t> &s) : domain_t(error(), *s) {}

  // RHS
  struct rhs : tuple<> {};
  domain_t(rhs, const domain_t &s)
      : t(1.0), // dt/dt
        grids(s.grids.size()) {
    auto bm = make_client<cell_t>(cell_t::boundary(), s.t,
                                  defs->xmin - 0.5 * defs->dx);
    auto bp = make_client<cell_t>(cell_t::boundary(), s.t,
                                  defs->xmax + 0.5 * defs->dx);
    for (size_t i = 0; i < grids.size(); ++i) {
      auto cm = i == 0 ? bm : client<cell_t>(async(grid_get_boundary_action(),
                                                   grids[i - 1], true));
      auto cp = i == grids.size() - 1
                    ? bp
                    : client<cell_t>(async(grid_get_boundary_action(),
                                           grids[i + 1], false));
      grids[i] = client<grid_t>(async(grid_rhs_action(), s.grids[i], cm, cp));
    }
  }
  domain_t(rhs, const client<domain_t> &s) : domain_t(rhs(), *s) {}
};
// RPC_COMPONENT(domain_t);

// Output
auto operator<<(ostream &os, const client<domain_t> &d) -> ostream & {
  return os << d.make_local()->output();
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
auto rk2(const shared_ptr<memoized_t> &m) -> client<domain_t> {
  const client<domain_t> &s0 = m->state;
  const client<domain_t> &r0 = m->rhs;
  // Step 1
  auto s1 = make_client<domain_t>(domain_t::axpy(), 0.5 * defs->dt, r0, s0);
  auto r1 = make_client<domain_t>(domain_t::rhs(), s1);
  // Step 2
  return make_client<domain_t>(domain_t::axpy(), defs->dt, r1, s0);
}

// Output
auto do_info_output(const shared_future<ostream *> &fos,
                    const shared_ptr<memoized_t> &m) -> ostream * {
  RPC_ASSERT(server->rank() == 0);
  const shared_ptr<domain_t> &s = m->state.get();
  m->error.get();
  const norm_t &en = m->error_norm.get();
  auto cell_size = cell_t().norm().count;
  auto ncells = en.count / cell_size;
  ostream *os = fos.get();
  *os << "n=" << m->n << " t=" << s->t << " "
      << "ncells: " << ncells << " "
      << "L2-norm[error]: " << en.norm2() << "\n" << flush;
  return os;
}

auto info_output(shared_future<ostream *> fos, const shared_ptr<memoized_t> &m)
    -> shared_future<ostream *> {
  if (do_this_time(m->n, defs->info_every))
    fos = async(do_info_output, fos, m);
  return fos;
}

auto do_file_output(const shared_future<ostream *> fos,
                    const shared_ptr<memoized_t> &m) -> ostream * {
  RPC_ASSERT(server->rank() == 0);
  const norm_t &en = m->error_norm.get();
  auto cell_size = cell_t().norm().count;
  auto ncells = en.count / cell_size;
  ostream *os = fos.get();
  *os << "State: " << m->state << "RHS: " << m->rhs << "ncells: " << ncells
      << "\n"
      << "L2-norm[error]: " << en.norm2() << "\n"
      << "\n" << flush;
  return os;
}

auto file_output(shared_future<ostream *> fos, const shared_ptr<memoized_t> &m)
    -> shared_future<ostream *> {
  if (do_this_time(m->n, defs->file_every))
    fos = async(do_file_output, fos, m);
  return fos;
}

////////////////////////////////////////////////////////////////////////////////

// Driver

auto get_thread_stats() -> string {
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

auto get_server_stats() -> string {
  ostringstream os;
  rpc::server_base::stats_t server_stats = rpc::server->get_stats();
  os << "[" << rpc::server->rank() << "] Server statistics:\n"
     << "   messages sent: " << server_stats.messages_sent << "\n"
     << "   messages received: " << server_stats.messages_received << "\n";
  return os.str();
}
RPC_ACTION(get_server_stats);

struct stats_t {
  struct snapshot_t {
    rpc::server_base::stats_t server_stats;
    rpc::thread_stats_t thread_stats;
    decltype(std::chrono::high_resolution_clock::now()) time;
  };
  snapshot_t start_, stop_;
  static auto snapshot(snapshot_t &dest) {
    dest.server_stats = rpc::server->get_stats();
    dest.thread_stats = rpc::get_thread_stats();
    dest.time = std::chrono::high_resolution_clock::now();
  }
  auto start() { snapshot(start_); }
  auto stop() { snapshot(stop_); }
  stats_t() { start(); }
  auto time_elapsed() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               stop_.time - start_.time).count() /
           1.0e+9;
  }
  auto output(ostream &os) const -> ostream & {
    auto messages_sent =
        stop_.server_stats.messages_sent - start_.server_stats.messages_sent;
    auto messages_received = stop_.server_stats.messages_received -
                             start_.server_stats.messages_received;
    auto threads_started = stop_.thread_stats.threads_started -
                           start_.thread_stats.threads_started;
    auto threads_stopped = stop_.thread_stats.threads_stopped -
                           start_.thread_stats.threads_stopped;
    return os << "   Messages: sent: " << messages_sent
              << ", received: " << messages_received << "\n"
              << "   Threads: started: " << threads_started
              << ", stopped: " << threads_stopped << "\n"
              << "   Time: " << time_elapsed() << " sec\n";
  }
};
auto operator<<(ostream &os, const stats_t &stats) -> ostream & {
  return stats.output(os);
}

auto rpc_main(int argc, char **argv) -> int {
  // Setup
  stats_t sstats;

  set_all_defs(
      make_shared<defs_t>(server->size(), thread::hardware_concurrency()));

  shared_future<ostream *> fio = make_ready_future<ostream *>(&cout);
  ostringstream filename;
  filename << "wave." << rpc::server->size() << ".txt";
  ofstream file(filename.str(), ios_base::trunc);
  shared_future<ostream *> ffo = make_ready_future<ostream *>(&file);

  sstats.stop();
  cout << "Setup:\n" << sstats;
  ptrdiff_t nthreads = server->size() * thread::hardware_concurrency();

  // Initialization
  stats_t istats;

  auto s = make_client<domain_t>(domain_t::initial(), defs->tmin);
  auto m = make_shared<memoized_t>(0, s);
  fio = info_output(fio, m);
  ffo = file_output(ffo, m);

  if (do_this_time(m->n, defs->wait_every)) {
    // Rate limiter
    s.wait();
    fio.wait();
    ffo.wait();
    fio.get()->flush();
    ffo.get()->flush();
  }

  istats.stop();
  cout << "Initialization:\n" << istats;
  ptrdiff_t ncells = defs->ncells;
  double time_init_cell = istats.time_elapsed() * nthreads / ncells;
  cout << "   num cells: " << ncells << "\n"
       << "   cpu time/cell: " << time_init_cell * 1.0e+9 << " nsec\n";

  // Evolution
  stats_t estats;

  while ((defs->nsteps < 0 || m->n < defs->nsteps) &&
         (defs->tmax < 0.0 || m->state->t < defs->tmax + 0.5 * defs->dt)) {

    s = rk2(m);
    m = make_shared<memoized_t>(m->n + 1, s);
    fio = info_output(fio, m);
    ffo = file_output(ffo, m);

    if (do_this_time(m->n, defs->wait_every)) {
      // Rate limiter
      s.wait();
      fio.wait();
      ffo.wait();
      fio.get()->flush();
      ffo.get()->flush();
    }
  }

  fio.wait();
  ffo.wait();

  estats.stop();
  cout << "Evolution:\n" << estats;
  ptrdiff_t nrhsevals = m->n * 2; // factor 2 for RK2
  double time_rhs = estats.time_elapsed() / nrhsevals;
  double time_rhs_cell = time_rhs * nthreads / ncells;
  cout << "   num RHS evals: " << nrhsevals << "\n"
       << "   wall time/RHS: " << time_rhs << " sec\n"
       << "   cpu time/RHS/cell: " << time_rhs_cell * 1.0e+9 << " nsec\n";

  // double elapsed_init =
  //     std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
  //     1.0e+9;
  // double elapsed_calc =
  //     std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() /
  //     1.0e+9;
  // cout << "Elapsed time (init): " << elapsed_init << " sec\n";
  // cout << "Elapsed time (calc): " << elapsed_calc << " sec\n";

  // for (int p = 0; p < rpc::server->size(); ++p) {
  //   cout << sync(rlaunch::sync, p, get_thread_stats_action());
  // }
  // for (int p = 0; p < rpc::server->size(); ++p) {
  //   cout << sync(rlaunch::sync, p, get_server_stats_action());
  // }

  file.close();

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

#define RPC_IMPLEMENT_NAMED_ACTION(action)                                     \
  RPC_CLASS_EXPORT(action::evaluate);                                          \
  RPC_CLASS_EXPORT(action::finish);

// RPC_IMPLEMENT_NAMED_ACTION(RPC_IDENTITY_TYPE((cxx::detail::bind_action<
//     cxx::tree<grid_t, vector_, rpc::client>,
//     cxx::tree_stencil_functor<grid_rhs_action, grid_get_boundary_action,
//     true,
//                               grid_t, vector_, rpc::client,
//                               cell_t>::get_boundary_tree_action,
//     bool>)));
