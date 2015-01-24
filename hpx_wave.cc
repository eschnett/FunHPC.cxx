#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

using hpx::async;
using hpx::finalize;
using hpx::find_all_localities;
using hpx::find_here;
using hpx::future;
using hpx::get_locality_id;
using hpx::id_type;
using hpx::launch;
using hpx::make_ready_future;
using hpx::naming::get_locality_from_id;
using hpx::shared_future;

using boost::make_shared;
using boost::shared_ptr;

using std::cerr;
using std::cout;
using std::enable_if;
using std::false_type;
using std::flush;
using std::forward;
using std::ios_base;
using std::max;
using std::min;
using std::move;
using std::numeric_limits;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::ptrdiff_t;
using std::string;
using std::true_type;
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
const ptrdiff_t rho = 1; // 100; // resolution scale

const double tmin = 0.0;
const double xmin = 0.0;
const double xmax = 1.0;

const ptrdiff_t ncells = 10 * rho;
const double dx = (xmax - xmin) / ncells;
double x(ptrdiff_t i) { return xmin + (i + 0.5) * dx; }

const double cfl = 0.5;
const double dt = cfl * dx;

const int nsteps = 2 * 1; // 2 * 100; // 2 * ncells
const int wait_every = 1;
const int info_every = nsteps / 10;
const int file_every = 0; // nsteps;

const ptrdiff_t ncells_per_grid = 10;
}

////////////////////////////////////////////////////////////////////////////////

// Components and clients

template <typename T>
class HPX_COMPONENT_EXPORT component
    : public hpx::components::managed_component_base<component<T> > {
  typedef hpx::components::managed_component_base<component<T> > component_base;
  shared_ptr<T> ptr;

public:
  component() {}
  component(const shared_ptr<T> &ptr) : ptr(ptr) {}
  component(shared_ptr<T> &&ptr) : ptr(std::move(ptr)) {}
  shared_ptr<T> get() const { return ptr; }
  HPX_DEFINE_COMPONENT_CONST_ACTION_TPL(component, get, get_action);
};

template <typename T> void swap(component<T> &a, component<T> &b) { a.swap(b); }

template <typename T>
class client : public hpx::components::client_base<
                   client<T>, hpx::components::stub_base<component<T> > > {
  typedef hpx::components::client_base<
      client<T>, hpx::components::stub_base<component<T> > > client_base;

public:
  using client_base::operator=;
  using client_base::create;
  using client_base::get_gid;
  client() {}
  client(const client_base &base) : client_base(base) {}
  client(client_base &&base) : client_base(move(base)) {}
  client(const id_type &gid) : client_base(gid) {}
  client(future<id_type> &&gid) : client_base(move(gid)) {}
  client(const shared_future<id_type> &gid) : client_base(gid) {}
  client(future<client> &&other) : client_base(move(other)) {}
  client(const shared_future<client> &other) : client_base(other) {}
  //
  id_type get_loc() const { return get_locality_from_id(get_gid()); }
  bool is_local() const { return get_loc() == find_here(); }
  void wait() const { get_gid(); }
  shared_ptr<T> get() const {
    assert(is_local());
    return typename component<T>::get_action()(get_gid());
  }
  T &operator*() const { return *get(); }
  auto operator -> () const -> decltype(this -> get()) { return get(); }
  client make_local() const {
    return async([](const client &self) {
                   return create(
                       find_here(),
                       typename component<T>::get_action()(self.get_gid()));
                 },
                 *this);
  }
};

template <typename T, typename... As> client<T> make_client(As &&... args) {
  return client<T>::create(find_here(),
                           boost::make_shared<T>(forward<As>(args)...));
}

template <typename T, typename... As>
client<T> make_remote_client(const id_type &loc, As &&... args) {
  return client<T>::create(loc, boost::make_shared<T>(forward<As>(args)...));
}

// namespace detail {
// template <typename T> struct is_client : false_type {};
// template <typename T> struct is_client<client<T> > : true_type {};
// template <typename T>
// auto extract_gid(T &&x)
//     -> typename enable_if<!is_client<T>::value,
// decltype(forward<T>(x))> {
//   return forward<T>(x);
// }
// template <typename T>
// auto extract_gid(T &&x)
//     -> typename enable_if<is_client<T>::value,
//                           decltype(forward<T>(x).get_gid())> {
//   return forward<T>(x).get_gid();
// }
// }
// template <typename... As>
// auto async_(As &&... args)
//     -> decltype(async(detail::extract_gid(forward<As>(args))...)) {
//   return async(detail::extract_gid(forward<As>(args))...);
// }

////////////////////////////////////////////////////////////////////////////////

// A norm

struct norm_t {
  double sum, sum2, count;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int version) {
    ar &sum &sum2 &count;
  }

public:
  norm_t() : sum(0.0), sum2(0.0), count(0.0) {}
  norm_t(double val) : sum(val), sum2(pow(val, 2)), count(1.0) {}
  norm_t(const norm_t &x, const norm_t &y)
      : sum(x.sum + y.sum), sum2(x.sum2 + y.sum2), count(x.count + y.count) {}
  double avg() const { return sum / count; }
  double norm2() const { return sqrt(sum2 / count); }
};

////////////////////////////////////////////////////////////////////////////////

// A cell -- a container holding data, no intelligence here

struct cell_t {
  static constexpr double nan = numeric_limits<double>::quiet_NaN();

  double u;
  double rho;
  double v;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int verstion) {
    ar &u &rho &v;
  }

public:
  // For safety
  cell_t() : u(nan), rho(nan), v(nan) {}

  // Output
  ostream &output(ostream &os) const {
    assert(get_locality_id() == 0);
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

// Output
ostream &operator<<(ostream &os, const cell_t &c) { return c.output(os); }

norm_t cell_norm(const cell_t &c) { return c.norm(); }

////////////////////////////////////////////////////////////////////////////////

// Each grid lives on a process

struct grid_t {
  double t;
  ptrdiff_t imin, imax; // spatial indices
  vector<cell_t> cells;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int version) {
    ar &t &imin &imax &cells;
  }

public:
  grid_t() {} // only for serialization
  grid_t(double t, ptrdiff_t imin, ptrdiff_t imax)
      : t(t), imin(imin), imax(imax), cells(imax - imin) {}
  grid_t(const client<grid_t> &g) : grid_t(g->t, g->imin, g->imax) {}

private:
  void set(ptrdiff_t i, const cell_t &c) {
    assert(i >= imin && i < imax);
    cells[i - imin] = c;
  }

public:
  const cell_t &get(ptrdiff_t i) const {
    assert(i >= imin && i < imax);
    return cells[i - imin];
  }

  cell_t get_boundary(bool face_upper) const {
    return get(face_upper ? imax - 1 : imin);
  }

  // Output
  ostream &output(ostream &os) const {
    assert(get_locality_id() == 0);
    os << "grid: t=" << t << "\n";
    for (ptrdiff_t i = imin; i < imax; ++i) {
      os << "   i=" << i << " x=" << defs::x(i) << " " << get(i) << "\n";
    }
    return os;
  }

  // Linear combination
  struct axpy : empty {};
  grid_t(axpy, double a, client<grid_t> x, client<grid_t> y) : grid_t(y) {
    t = a * x->t + y->t;
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::axpy(), a, x->get(i), y->get(i)));
    }
  }

  // Norm
  norm_t norm() const {
    norm_t n;
    for (ptrdiff_t i = imin; i < imax; ++i) {
      n = norm_t(n, cell_norm(get(i)));
    }
    return n;
  }

  // Initial condition
  struct initial : empty {};
  grid_t(initial, double t, ptrdiff_t imin, ptrdiff_t imax)
      : grid_t(t, imin, imax) {
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::initial(), t, defs::x(i)));
    }
  }

  // Error
  struct error : empty {};
  grid_t(error, client<grid_t> s) : grid_t(s) {
    for (ptrdiff_t i = imin; i < imax; ++i) {
      set(i, cell_t(cell_t::error(), s->get(i), t, defs::x(i)));
    }
  }

  // RHS
  struct rhs : empty {};
  grid_t(rhs, client<grid_t> s, shared_future<cell_t> bm,
         shared_future<cell_t> bp)
      : grid_t(s) {
    t = 1.0; // dt/dt
    for (ptrdiff_t i = imin; i < imax; ++i) {
      const cell_t &c = s->get(i);
      // choose left neighbour
      ptrdiff_t im = i - 1;
      cell_t cm;
      if (im >= imin) { // same grid (interior)
        cm = s->get(im);
      } else if (im >= 0) { // other grid (ghost)
        cm = bm.get();
      } else { // boundary
        cm = cell_t(cell_t::boundary(), s->t, defs::x(im));
      }
      // choose right neighbour
      ptrdiff_t ip = i + 1;
      cell_t cp;
      if (ip < imax) { // same grid (interior)
        cp = s->get(ip);
      } else if (ip < defs::ncells) { // other grid (ghost)
        cp = bp.get();
      } else { // boundary
        cp = cell_t(cell_t::boundary(), s->t, defs::x(ip));
      }
      set(i, cell_t(cell_t::rhs(), c, cm, cp));
    }
  }
};
typedef component<grid_t> grid_t_component;
HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(
    hpx::components::managed_component<component<grid_t> >, grid_t_component);

HPX_REGISTER_ACTION_DECLARATION(component<grid_t>::get_action,
                                grid_t_component_get_action);
HPX_REGISTER_ACTION(component<grid_t>::get_action, grid_t_component_get_action);

// Output
ostream &operator<<(ostream &os, const client<grid_t> &g) {
  return g->output(os);
}

cell_t grid_get_boundary(const client<grid_t> &g, bool face_upper) {
  return g->get_boundary(face_upper);
}
HPX_PLAIN_ACTION(grid_get_boundary);

// Norm
norm_t grid_norm(const client<grid_t> &g) { return g->norm(); }
HPX_PLAIN_ACTION(grid_norm);

////////////////////////////////////////////////////////////////////////////////

// The domain is distributed over multiple processes

struct domain_t {

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
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, unsigned int version) {
    ar &t &grids;
  }
  domain_t() {} // only for serialization
public:
  // Note: The grids are not allocated in the constructor; this has to
  // be done explicitly.
  domain_t(double t) : t(t), grids(ngrids()) {}
  domain_t(const client<domain_t> &d) : t(d->t), grids(ngrids()) {}

private:
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

  // Wait until all grids are ready
  void wait() const {
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      get(i).wait();
    }
  }

  // Output
  ostream &output(ostream &os) const {
    assert(get_locality_id() == 0);
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
      set(i, make_remote_client<grid_t>(y->get(i).get_loc(), grid_t::axpy(), a,
                                        x->get(i), y->get(i)));
    }
  }

  // Norm
  norm_t norm() const {
    // TODO: Use a proper reduction operation
    vector<future<norm_t> > fns;
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      fns.push_back(
          async(grid_norm_action(), get(i).get_loc(), get(i).get_gid()));
    }
    norm_t n;
    for (auto &fn : fns)
      n = norm_t(n, fn.get());
    return n;
  }

  // Initial condition
  struct initial : empty {};
  domain_t(initial, double t) : domain_t(t) {
    auto locs = find_all_localities();
    ptrdiff_t nlocs = locs.size();
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      // Choose a domain decomposition
      id_type loc = locs.at(mod_floor(ptrdiff_t(get_locality_id()) + i, nlocs));
      ptrdiff_t imin = i * defs::ncells_per_grid;
      ptrdiff_t imax = min((i + 1) * defs::ncells_per_grid, defs::ncells);
      set(i, make_remote_client<grid_t>(loc, grid_t::initial(), t, imin, imax));
    }
  }

  // Error
  struct error : empty {};
  domain_t(error, const client<domain_t> &s) : domain_t(s) {
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      const client<grid_t> &si = s->get(i);
      set(i, make_remote_client<grid_t>(si.get_loc(), grid_t::error(), si));
    }
  }

  // RHS
  struct rhs : empty {};
  domain_t(rhs, const client<domain_t> &s) : domain_t(s) {
    t = 1.0; // dt/dt
    const shared_future<cell_t> fake = make_ready_future(cell_t());
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      const client<grid_t> &si = s->get(i);
      // left buondary
      ptrdiff_t im = i - 1;
      shared_future<cell_t> cm;
      if (im >= 0) {
        const client<grid_t> &sim = s->get(im);
        cm = async(grid_get_boundary_action(), sim.get_loc(), sim.get_gid(),
                   true);
      } else {
        cm = fake;
      }
      // right boundary
      ptrdiff_t ip = i + 1;
      shared_future<cell_t> cp;
      if (ip < ngrids()) {
        const client<grid_t> &sip = s->get(ip);
        cp = async(grid_get_boundary_action(), sip.get_loc(), sip.get_gid(),
                   false);
      } else {
        cp = fake;
      }
      set(i,
          make_remote_client<grid_t>(si.get_loc(), grid_t::rhs(), si, cm, cp));
    }
  }
};
typedef component<domain_t> domain_t_component;
HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(
    hpx::components::managed_component<component<domain_t> >,
    domain_t_component);

// Output
ostream &operator<<(ostream &os, const client<domain_t> &d) {
  return d->output(os);
}

// Norm
norm_t domain_norm(const client<domain_t> &x) { return x->norm(); }

////////////////////////////////////////////////////////////////////////////////

// Memoized data for the current iteration

struct memoized_t {
  ptrdiff_t n;
  client<domain_t> state;
  client<domain_t> rhs;
  client<domain_t> error;
  shared_future<norm_t> error_norm;
  memoized_t(ptrdiff_t n, const client<domain_t> &state) : n(n), state(state) {
    rhs = client<domain_t>(async(launch::deferred, [state]() {
      return make_client<domain_t>(domain_t::rhs(), state);
    }));
    error = client<domain_t>(async(launch::deferred, [state]() {
      return make_client<domain_t>(domain_t::error(), state);
    }));
    error_norm = async(launch::deferred, domain_norm, error);
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
  assert(get_locality_id() == 0);
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
  assert(get_locality_id() == 0);
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

int main(int argc, char **argv) {
  ostringstream filename;
  auto locs = find_all_localities();
  auto nlocs = locs.size();
  filename << "hpx_wave." << nlocs << ".txt";
  ofstream file(filename.str(), ios_base::trunc);

  auto t0 = std::chrono::high_resolution_clock::now();

  int n = 0;
  auto s = make_client<domain_t>(domain_t::initial(), defs::tmin);
  auto m = boost::make_shared<memoized_t>(n, s);
  auto fio = info_output(make_ready_future<ostream *>(&cout), m);
  auto ffo = file_output(make_ready_future<ostream *>(&file), m);

  while (n < defs::nsteps) {

    if (defs::wait_every != 0 && n > 0 && n % defs::wait_every == 0) {
      // Rate limiter
      s.wait();
    }

    s = rk2(m);
    ++n;
    m = boost::make_shared<memoized_t>(n, s);
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

  file.close();

  return 0;
}
