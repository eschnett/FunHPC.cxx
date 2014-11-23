// The scalar wave equation in multiple dimensions. This uses a vector
// instead of a tree to store the remote futures to the grids, and is
// thus not scalable.

#include "rpc.hh"

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_grid.hh"
#include "cxx_iota.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"

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

using cxx::boundaries;
using cxx::div_ceil;
using cxx::div_floor;
using cxx::fmap;
using cxx::fmap2;
using cxx::foldMap;
using cxx::iota;
// using cxx::iota_range_t;
// using cxx::range_t;
using cxx::stencil_fmap;
using cxx::vect;

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

using std::ceil;
using std::cerr;
using std::cout;
using std::flush;
using std::ofstream;
using std::ios_base;
using std::lrint;
using std::make_shared;
using std::max;
using std::min;
using std::move;
using std::numeric_limits;
using std::ostream;
using std::ostringstream;
using std::pow;
using std::ptrdiff_t;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;

////////////////////////////////////////////////////////////////////////////////

// Define some Monoids

auto tuple_mempty() { return tuple<>(); }
RPC_ACTION(tuple_mempty);
auto tuple_mappend(tuple<>, tuple<>) { return tuple<>(); }
RPC_ACTION(tuple_mappend);

auto string_mempty() { return string(); }
RPC_ACTION(string_mempty);
auto string_mappend(const string &xs, const string &ys) { return xs + ys; }
RPC_ACTION(string_mappend);

////////////////////////////////////////////////////////////////////////////////

// Global definitions, a poor man's parameter file

constexpr ptrdiff_t dim = 1;
typedef cxx::index<dim> vindex;
typedef vect<double, dim> vdouble;

struct defs_t {
#if 1                       // benchmark
  const ptrdiff_t rho = 64; // resolution scale
  const ptrdiff_t ncells_per_grid = 4;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.25;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = 8;
  const ptrdiff_t wait_every = 0;
  const ptrdiff_t info_every = 0;
  const ptrdiff_t file_every = -1;
#elif 0 // test
  const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t ncells_per_grid = 4;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.25;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = -1;
  const ptrdiff_t wait_every = 8;
  const ptrdiff_t info_every = 8;
  const ptrdiff_t file_every = 0;
#elif 0 // debug
  const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t ncells_per_grid = 4;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.25;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = 8;
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
      : ncells(ncells_per_grid *
               lrint(pow(double(rho *nprocs *nthreads), 1 / double(dim)))),
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

  vdouble x;
  double u;
  double rho;
  vdouble v;

private:
  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) { ar(x, u, rho, v); }

public:
  // For safety
  cell_t() : x(vdouble::set1(nan)), u(nan), rho(nan), v(vdouble::set1(nan)) {}

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
  auto norm() const {
    return norm_t(u) + norm_t(rho) + foldMap([](double x) { return norm_t(x); },
                                             norm_mappend, norm_mempty(), v);
  }

  // Analytic solution
  struct analytic : tuple<> {};
  cell_t(analytic, double t, vdouble x) {
    double omega = std::sqrt(double(dim));
    this->x = x;
    u = sin(2 * M_PI * omega * t);
    rho = 2 * M_PI * omega * cos(2 * M_PI * omega * t);
    v = vdouble::set1(sin(2 * M_PI * omega * t));
    for (ptrdiff_t j = 0; j < dim; ++j) {
      u *= sin(2 * M_PI * x[j]);
      rho *= sin(2 * M_PI * x[j]);
      for (ptrdiff_t i = 0; i < dim; ++i)
        v = v.set(i, v[i] * (j == i ? 2 * M_PI * cos(2 * M_PI * x[j])
                                    : sin(2 * M_PI * x[j])));
    }
  }

  // Initial condition
  struct initial : tuple<> {};
  cell_t(initial, double t, vdouble x) : cell_t(analytic(), t, x) {}

  // Boundary condition
  struct boundary : tuple<> {};
  cell_t(boundary, double t, vdouble x) : cell_t(analytic(), t, x) {}
  // RPC_DECLARE_CONSTRUCTOR(cell_t, boundary, double, vdouble);

  // Error
  struct error : tuple<> {};
  cell_t(error, const cell_t &c, double t)
      : cell_t(axpy(), -1.0, cell_t(analytic(), t, c.x), c) {}

  // RHS
  struct rhs : tuple<> {};
  cell_t(rhs, const cell_t &c, const boundaries<cell_t, dim> &bs) {
    x = vdouble::zero(); // dx/dt
    u = c.rho;
    rho = 0.0;
    for (ptrdiff_t i = 0; i < dim; ++i)
      rho += (bs(i, true).v[i] - bs(i, false).v[i]) / (2 * defs->dx);
    for (ptrdiff_t i = 0; i < dim; ++i)
      v = v.set(i, (bs(i, true).rho - bs(i, false).rho) / (2 * defs->dx));
  }

  struct get_boundary : tuple<> {};
  cell_t(get_boundary, const cell_t &c, ptrdiff_t dir, bool face) : cell_t(c) {}
};
RPC_COMPONENT(cell_t);
// RPC_IMPLEMENT_CONSTRUCTOR(cell_t, cell_t::boundary, double, vdouble);

// Output
auto operator<<(ostream &os, const cell_t &c) -> ostream & {
  return os << c.output();
}

////////////////////////////////////////////////////////////////////////////////

// Each grid lives on a process

template <typename T> using grid_ = cxx::grid<T, dim>;

class grid_t {
  typedef grid_<cell_t> cells_t;
  cells_t cells;

  static auto grid2cells(const boundaries<grid_t, dim> &bs) {
    boundaries<cells_t, dim> newbs;
    for (ptrdiff_t face = 0; face < 2; ++face)
      for (ptrdiff_t dir = 0; dir < dim; ++dir)
        newbs(dir, face) = bs(dir, face).cells;
    return newbs;
  }

  static auto x(vindex i) {
    return vdouble::set1(defs->xmin) +
           (vdouble(i) + vdouble::set1(0.5)) * defs->dx;
  }

  friend class cereal::access;
  template <typename Archive> auto serialize(Archive &ar) { ar(cells); }

public:
  // only for serialization
  grid_t()
      : cells(cells_t::iota(), [](const vindex &i) { return cell_t(); },
              cxx::grid_region<dim>()) {}

  // auto get(vindex i) const -> const cell_t & {
  //   assert(all_of(i >= imin && i < imax));
  //   return cells[i - imin];
  // }

  // auto get_boundary(ptrdiff_t dir, bool face) const {
  //   return cells_t(cells_t::boundary(), cells, dir, face);
  // }

  // Wait until the grid is ready
  auto wait() const { return tuple<>(); }

  // Output
  auto output() const {
    ostringstream os;
    os << cells;
    return os.str();
  }

  // Linear combination
  struct axpy : tuple<> {};
  grid_t(axpy, double a, const grid_t &x, const grid_t &y)
      : cells(fmap2([a](const cell_t &x, const cell_t &y) {
                      return cell_t(cell_t::axpy(), a, x, y);
                    },
                    x.cells, y.cells)) {}

  // Norm
  auto norm() const {
    return foldMap([](const cell_t &c) { return c.norm(); },
                   [](const norm_t &x, const norm_t &y) { return x + y; },
                   norm_t(), cells);
  }

  // Initial condition
  struct initial : tuple<> {};
  grid_t(initial, double t, vindex imin)
      : cells(iota<grid_>(
            [t](const vindex &i) { return cell_t(cell_t::initial(), t, x(i)); },
            cxx::grid_region<dim>(
                imin, min(imin + vindex::set1(defs->ncells_per_grid),
                          vindex::set1(defs->ncells))))) {}

  // Boundary condition
  struct boundary : tuple<> {};
  grid_t(boundary, const grid_t &g, double t, ptrdiff_t dir, bool face)
      : cells(fmap([t, dir, face](const cell_t &c) {
                     auto vn =
                         vdouble(ptrdiff_t(!face ? -1 : +1) * vindex::dir(dir));
                     return cell_t(cell_t::boundary(), t, c.x + vn * defs->dx);
                   },
                   cells_t(cells_t::boundary(), g.cells, dir, face))) {}

  // Error
  struct error : tuple<> {};
  grid_t(error, const grid_t &g, double t)
      : cells(
            fmap([t](const cell_t &c) { return cell_t(cell_t::error(), c, t); },
                 g.cells)) {}

  // RHS
  struct rhs : tuple<> {};
  grid_t(rhs, const grid_t &g, const boundaries<grid_t, dim> &bs)
      : cells(stencil_fmap(
            [](const cell_t &c, const boundaries<cell_t, dim> &bs) {
              return cell_t(cell_t::rhs(), c, bs);
            },
            [](const cell_t &c, ptrdiff_t dir, bool face) {
              return cell_t(cell_t::get_boundary(), c, dir, face);
            },
            g.cells, grid2cells(bs))) {}

  struct get_boundary : tuple<> {};
  grid_t(get_boundary, const grid_t &g, ptrdiff_t dir, bool face)
      : cells(fmap([dir, face](const cell_t &c) {
                     return cell_t(cell_t::get_boundary(), c, dir, face);
                   },
                   cells_t(cells_t::boundary(), g.cells, dir, face))) {}
};
RPC_COMPONENT(grid_t);

// Output
auto operator<<(ostream &os, const grid_t &g) -> ostream & {
  return os << g.output();
}
// auto operator<<(ostream &os, const client<grid_t> &g) -> ostream & {
//   return os << *g.make_local();
// }

// auto grid_get_boundary(const client<grid_t> &g, ptrdiff_t dir, bool face) {
//   return g->get_boundary(dir, face);
// }
// RPC_ACTION(grid_get_boundary);

auto grid_wait_foldMap(const client<grid_t> &g) { return g->wait(); }
RPC_ACTION(grid_wait_foldMap);

auto grid_output_foldMap(const client<grid_t> &g) { return g->output(); }
RPC_ACTION(grid_output_foldMap);

// Note: Arguments re-ordered
auto grid_axpy(const client<grid_t> &x, const client<grid_t> &y, double a) {
  auto yl = y.make_local();
  return make_client<grid_t>(grid_t::axpy(), a, *x, *yl);
}
RPC_ACTION(grid_axpy);

auto grid_norm_foldMap(const client<grid_t> &g) { return g->norm(); }
RPC_ACTION(grid_norm_foldMap);

// Note: Arguments re-ordered
auto grid_initial(vindex ipos, double t) {
  return make_client<grid_t>(grid_t::initial(), t,
                             ipos * defs->ncells_per_grid);
}
RPC_ACTION(grid_initial);

auto grid_boundary(const client<grid_t> &g, double t, ptrdiff_t dir,
                   bool face) {
  return make_client<grid_t>(grid_t::boundary(), *g, t, dir, face);
}
RPC_ACTION(grid_boundary);

auto grid_error(const client<grid_t> &g, double t) {
  return make_client<grid_t>(grid_t::error(), *g, t);
}
RPC_ACTION(grid_error);

auto grid_rhs(const client<grid_t> &g,
              const boundaries<client<grid_t>, dim> &bs) {
  boundaries<client<grid_t>, dim> tmpbs;
  for (ptrdiff_t face = 0; face < 2; ++face)
    for (ptrdiff_t dir = 0; dir < dim; ++dir)
      tmpbs(dir, face) = bs(dir, face).make_local();
  boundaries<grid_t, dim> newbs;
  for (ptrdiff_t face = 0; face < 2; ++face)
    for (ptrdiff_t dir = 0; dir < dim; ++dir)
      newbs(dir, face) = *tmpbs(dir, face);
  return make_client<grid_t>(grid_t::rhs(), *g, newbs);
}
RPC_ACTION(grid_rhs);

auto grid_get_boundary(const client<grid_t> &g, ptrdiff_t dir, bool face) {
  return make_client<grid_t>(grid_t::get_boundary(), *g, dir, face);
}
RPC_ACTION(grid_get_boundary);

////////////////////////////////////////////////////////////////////////////////

// The domain is distributed over multiple processes

RPC_COMPONENT(grid_<grid_t>);

struct domain_t {

  double t;

  // static auto ngrids() -> ptrdiff_t {
  //   return div_ceil(defs->ncells, defs->ncells_per_grid);
  // }
  // static auto cell2grid(ptrdiff_t icell) -> ptrdiff_t {
  //   assert(icell >= 0 && icell < defs->ncells);
  //   return div_floor(icell, defs->ncells_per_grid);
  // }

  typedef grid_<client<grid_t> > grids_t;
  grids_t grids;

  // Wait until all grids are ready
  auto wait() const {
    return foldMap(grid_wait_foldMap_action(), tuple_mappend_action(),
                   tuple<>(), grids);
  }

  // Output
  auto output() const {
    ostringstream os;
    os << "domain: t=" << t << "\n"
       << foldMap(grid_output_foldMap_action(), string_mappend_action(),
                  string(), grids);
    return os.str();
  }

  // Linear combination
  struct axpy : tuple<> {};
  domain_t(axpy, double a, const domain_t &x, const domain_t &y)
      : t(a * x.t + y.t), grids(fmap2(grid_axpy_action(),
                                      // TODO: grid_t::axpy_action(),
                                      x.grids, y.grids, a)) {}
  domain_t(axpy, double a, const client<domain_t> &x, const client<domain_t> &y)
      : domain_t(axpy(), a, *x, *y) {}

  // Norm
  auto norm() const {
    return foldMap(grid_norm_foldMap_action(), norm_mappend_action(), norm_t(),
                   grids);
  }

  // Initial condition
  // (Also choose a domain decomposition)
  struct initial : tuple<> {};
  domain_t(initial, double t)
      : t(t),
        grids(iota<grid_>(
            grid_initial_action(),
            cxx::grid_region<dim>(
                vindex::zero(),
                vindex::set1(div_ceil(defs->ncells, defs->ncells_per_grid))),
            t)) {}

  // Boundary condition
  struct boundary : tuple<> {};
  domain_t(boundary, const domain_t &d, double t, ptrdiff_t dir, bool face)
      : t(t), grids(fmap(grid_boundary_action(),
                         grids_t(grids_t::boundary(), d.grids, dir, face), t,
                         dir, face)) {}

  auto all_boundaries() const {
    boundaries<grids_t, dim> bs;
    for (ptrdiff_t face = 0; face < 2; ++face)
      for (ptrdiff_t dir = 0; dir < dim; ++dir)
        bs(dir, face) =
            domain_t(domain_t::boundary(), *this, t, dir, face).grids;
    return bs;
  }

  // Error
  struct error : tuple<> {};
  domain_t(error, const domain_t &s)
      : t(s.t), grids(fmap(grid_error_action(), s.grids, t)) {}
  domain_t(error, const client<domain_t> &s) : domain_t(error(), *s) {}

  // RHS
  struct rhs : tuple<> {};
  domain_t(rhs, const domain_t &s)
      : t(1.0), // dt/dt
        grids(stencil_fmap(grid_rhs_action(), grid_get_boundary_action(),
                           s.grids, s.all_boundaries())) {}
  domain_t(rhs, const client<domain_t> &s) : domain_t(rhs(), *s) {}
};

// Output
auto operator<<(ostream &os, const domain_t &d) -> ostream & {
  return os << d.output();
}
auto operator<<(ostream &os, const client<domain_t> &d) -> ostream & {
  return os << *d.make_local();
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
  //     std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()
  //     /
  //     1.0e+9;
  // double elapsed_calc =
  //     std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()
  //     /
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
