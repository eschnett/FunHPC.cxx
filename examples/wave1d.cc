#include <fun/topology.hpp>
#include <fun/vector.hpp>
// #include <fun/proxy.hpp>
#include <fun/shared_future.hpp>
#include <funhpc/rexec.hpp>
#include <funhpc/main.hpp>

#include <fun/nested.hpp>
#include <fun/tree.hpp>

#include <fun/fun.hpp>

#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

// Parameters
struct parameters_t {
  std::ptrdiff_t ncells;
  const double xmin = 0.0;
  const double xmax = 1.0;
  double dx() const { return (xmax - xmin) / ncells; }

  std::ptrdiff_t nsteps;
  const double tmin = 0.0;
  std::ptrdiff_t icfl = 1;
  double dt() const { return dx() / icfl; }

  std::ptrdiff_t outinfo_every;
  std::ptrdiff_t outfile_every;
  std::string outfile_name;
};

parameters_t parameters;

// Norm

struct norm_t {
  double count, min, max, max_abs, sum, sum2, sum_abs;
  norm_t()
      : count(0), min(1.0 / 0.0), max(-1.0 / 0.0), max_abs(0), sum(0), sum2(0),
        sum_abs(0) {}
  norm_t(double x)
      : count(1), min(x), max(x), max_abs(std::fabs(x)), sum(x),
        sum2(std::pow(x, 2)), sum_abs(std::fabs(x)) {}
  norm_t(const norm_t &other) = default;
  norm_t(norm_t &&other) = default;
  norm_t &operator=(const norm_t &other) = default;
  norm_t &operator=(norm_t &&other) = default;
  norm_t &operator+=(const norm_t &other) {
    count += other.count;
    min = std::fmin(min, other.min);
    max = std::fmax(max, other.max);
    max_abs = std::fmax(max_abs, other.max_abs);
    sum += other.sum;
    sum2 += other.sum2;
    sum_abs += other.sum_abs;
    return *this;
  }
  norm_t operator+(const norm_t &other) const { return norm_t(*this) += other; }
  double avg() const { return sum / count; }
  double norm1() const { return sum_abs / count; }
  double norm2() const { return std::sqrt(sum2 / count); }
  double norm_inf() const { return max_abs; }
};

auto norm_add(const norm_t &x, const norm_t &y) { return x + y; }
auto norm_zero() { return norm_t(); }

// Cell

struct cell_t {
  double x;
  double u, rho, v;
};

std::ostream &operator<<(std::ostream &os, const cell_t &c) {
  return os << "cell_t{x=" << c.x << " u=" << c.u << " rho=" << c.rho
            << " v=" << c.v << "}";
}

auto cell_axpy(const cell_t &y, const cell_t &x, double alpha) {
  return cell_t{y.x, alpha * x.u + y.u, alpha * x.rho + y.rho,
                alpha * x.v + y.v};
}

auto cell_analytic(double t, double x) {
  // Standing wave: u(t,x) = cos(k t) sin(k x) with k = 2\pi
  auto k = 2 * M_PI / (parameters.xmax - parameters.xmin);
  auto u = std::cos(k * t) * std::sin(k * x);
  auto rho = -k * std::sin(k * t) * std::sin(k * x);
  auto v = k * std::cos(k * t) * std::cos(k * x);
  return cell_t{x, u, rho, v};
}

auto cell_init(double t, double x) { return cell_analytic(t, x); }

auto cell_error(const cell_t &c, double t) {
  auto a = cell_analytic(t, c.x);
  return cell_axpy(c, a, -1.0);
}

auto cell_norm(const cell_t &c) {
  return norm_t(c.u) + norm_t(c.rho) + norm_t(c.v);
}

auto cell_energy(const cell_t &c) {
  return parameters.dx() * (0.5 * std::pow(c.rho, 2) + 0.5 * std::pow(c.v, 2));
}

// Note: These Dirichlet boundaries are unstable
auto cell_boundary_dirichlet(double t, std::ptrdiff_t i) {
  return cell_analytic(t, i == 0 ? parameters.xmin - 0.5 * parameters.dx()
                                 : parameters.xmax + 0.5 * parameters.dx());
}

auto cell_boundary_reflecting(const cell_t &c, std::ptrdiff_t i) {
  return cell_t{i == 0 ? 2 * parameters.xmin - c.x : 2 * parameters.xmax - c.x,
                -c.u, -c.rho, c.v};
}

auto cell_rhs(const cell_t &c, const fun::connectivity<cell_t> &bs) {
  auto dx = parameters.dx();
  auto u_rhs = c.rho;
  auto rho_rhs = (-0.5 * fun::get<0>(bs).v + 0.5 * fun::get<1>(bs).v) / dx;
  auto v_rhs = (-0.5 * fun::get<0>(bs).rho + 0.5 * fun::get<1>(bs).rho) / dx;
  return cell_t{0.0, u_rhs, rho_rhs, v_rhs};
}

// Grid

template <typename T> using std_vector = std::vector<T>;
template <typename T>
using proxy_vector =
    adt::nested</*funhpc::proxy*/ qthread::shared_future, std_vector, T>;
template <typename T> using proxy_tree = adt::tree<proxy_vector, T>;

struct grid_t {
  double time;
  proxy_tree<cell_t> cells;
};

std::ostream &operator<<(std::ostream &os, const grid_t &g) {
  os << "grid_t{\n"
     << "  time=" << g.time << "\n"
     << "  cells=" << fun::to_string(g.cells) << "\n"
     << "}\n";
  return os;
}

auto grid_axpy(const grid_t &y, const grid_t &x, double alpha) {
  return grid_t{alpha * x.time + y.time,
                fun::fmap2(cell_axpy, y.cells, x.cells, alpha)};
}

auto grid_init(double t) {
  return grid_t{t, fun::iotaMap<proxy_tree>([t](std::ptrdiff_t i) {
                                              double x = parameters.xmin +
                                                         parameters.dx() *
                                                             (double(i) + 0.5);
                                              return cell_init(t, x);
                                            },
                                            parameters.ncells)};
}

auto grid_error(const grid_t &g) {
  return grid_t{g.time, fun::fmap(cell_error, g.cells, g.time)};
}

auto grid_norm(const grid_t &g) {
  return fun::foldMap(cell_norm, norm_add, norm_zero(), g.cells);
}

auto grid_energy(const grid_t &g) {
  return fun::foldMap(cell_energy, std::plus<double>(), 0.0, g.cells);
}

auto grid_boundary(const grid_t &g, std::ptrdiff_t i) {
  // return cell_boundary_dirichlet(g.time, i);
  return cell_boundary_reflecting(
      i == 0 ? fun::head(g.cells) : fun::last(g.cells), i);
}

auto grid_rhs(const grid_t &g) {
  return grid_t{
      1.0, fun::fmapTopo(
               cell_rhs, [](const cell_t &c, std::ptrdiff_t i) { return c; },
               g.cells, fun::connectivity<cell_t>(grid_boundary(g, 0),
                                                  grid_boundary(g, 1)))};
}

// State

struct state_t {
  std::ptrdiff_t iter;
  grid_t state;
  grid_t error;
  qthread::shared_future<norm_t> fnorm;
  qthread::shared_future<double> fenergy;
  grid_t rhs;
  state_t(std::ptrdiff_t iter, const grid_t &state)
      : iter(iter), state(state), error(grid_error(state)),
        fnorm(qthread::async(grid_norm, error)),
        fenergy(qthread::async(grid_energy, state)), rhs(grid_rhs(state)) {}
};

auto euler(const state_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  return grid_axpy(s0, r0, parameters.dt());
}

auto rk2(const state_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  auto s1 = grid_axpy(s0, r0, 0.5 * parameters.dt());
  auto r1 = grid_rhs(s1);
  return grid_axpy(s0, r1, parameters.dt());
}

// Output

int info_output(int token, const state_t &s) {
  if (s.iter % parameters.outinfo_every == 0 || s.iter == parameters.nsteps) {
    std::cout << "[" << s.iter << "] " << s.state.time << ": "
              << s.fnorm.get().norm2() << " " << s.fenergy.get() << "\n";
  }
  return token;
}

int file_output(int token, const state_t &s) {
  if (s.iter % parameters.outfile_every == 0 || s.iter == parameters.nsteps) {
    std::fstream fs;
    auto mode = s.iter == 0 ? std::ios::in | std::ios::out | std::ios::trunc
                            : std::ios::in | std::ios::out | std::ios::ate;
    fs.open(parameters.outfile_name, mode);
    fs << fun::foldMap2([t = s.state.time](const cell_t &cs, const cell_t &ce) {
                          std::ostringstream ss;
                          ss << t << "\t" << cs.x << "\t" << cs.u << "\t"
                             << cs.rho << "\t" << cs.v << "\t" << ce.u << "\t"
                             << ce.rho << "\t" << ce.v << "\t"
                             << cell_energy(cs) << "\n";
                          return ss.str();
                        },
                        [](const std::string &s1,
                           const std::string &s2) { return s1 + s2; },
                        std::string(), s.state.cells, s.error.cells) << "\n";
    fs.close();
  }
  return token;
}

// Driver

int funhpc_main(int argc, char **argv) {
  std::cout << "Wave1d\n";
  parameters.ncells = 100;
  parameters.nsteps = parameters.ncells * parameters.icfl;
  parameters.outinfo_every = parameters.nsteps / 10;
  parameters.outfile_every = parameters.nsteps / 20;
  parameters.outfile_name = "wave1d.tsv";
  qthread::shared_future<int> info_token = qthread::make_ready_future(0);
  qthread::shared_future<int> file_token = qthread::make_ready_future(0);
  state_t s(0, grid_init(parameters.tmin));
  info_token = fun::fmap(info_output, info_token, s);
  file_token = fun::fmap(file_output, file_token, s);
  while (s.iter < parameters.nsteps) {
    s = state_t(s.iter + 1, rk2(s));
    info_token = fun::fmap(info_output, info_token, s);
    file_token = fun::fmap(file_output, file_token, s);
  }
  info_token.wait();
  file_token.wait();
  std::cout << "Done.\n";
  return 0;
}
