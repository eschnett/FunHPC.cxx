#include <adt/dummy.hpp>
#include <cxx/funobj.hpp>
#include <fun/fun_decl.hpp>
#include <fun/nested_decl.hpp>
#include <fun/proxy.hpp>
#include <fun/shared_future.hpp>
#include <fun/tree_decl.hpp>
#include <fun/vector.hpp>
#include <funhpc/main.hpp>
#include <funhpc/rexec.hpp>

#include <fun/fun_impl.hpp>
#include <fun/nested_impl.hpp>
#include <fun/tree_impl.hpp>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

// Types

typedef std::ptrdiff_t int_t;
typedef double real_t;

// Parameters

struct parameters_t {
  int_t ncells;
  const real_t xmin = 0.0;
  const real_t xmax = 1.0;
  real_t dx;
  real_t dx_1;

  int_t nsteps;
  const real_t tmin = 0.0;
  const int_t icfl = 1;
  real_t dt;

  int_t outinfo_every;
  int_t outfile_every;
  std::string outfile_name;

  void setup() {
    dx = (xmax - xmin) / ncells;
    dx_1 = 1.0 / dx;
    dt = dx / icfl;
  }
};

parameters_t parameters;

// Norm

struct norm_t {
  real_t count, min, max, max_abs, sum, sum2, sum_abs;
  template <typename Archive> void serialize(Archive &ar) {
    ar(count, min, max, max_abs, sum, sum2, sum_abs);
  }

  norm_t()
      : count(0), min(1.0 / 0.0), max(-1.0 / 0.0), max_abs(0), sum(0), sum2(0),
        sum_abs(0) {}
  norm_t(real_t x)
      : count(1), min(x), max(x), max_abs(std::fabs(x)), sum(x),
        sum2(std::pow(x, 2)), sum_abs(std::fabs(x)) {}
  norm_t(const norm_t &other) = default;
  norm_t(norm_t &&other) = default;
  norm_t &operator=(const norm_t &other) = default;
  norm_t &operator=(norm_t &&other) = default;
  norm_t &operator+=(const norm_t &other) {
    count += other.count;
    min = std::min(min, other.min);
    max = std::max(max, other.max);
    max_abs = std::max(max_abs, other.max_abs);
    sum += other.sum;
    sum2 += other.sum2;
    sum_abs += other.sum_abs;
    return *this;
  }
  norm_t operator+(const norm_t &other) const { return norm_t(*this) += other; }
  real_t avg() const { return sum / count; }
  real_t norm1() const { return sum_abs / count; }
  real_t norm2() const { return std::sqrt(sum2 / count); }
  real_t norm_inf() const { return max_abs; }
};

auto norm_add(const norm_t &x, const norm_t &y) { return x + y; }
auto norm_zero() { return norm_t(); }

// Cell

struct cell_t {
  real_t x;
  real_t u, rho, v;
  template <typename Archive> void serialize(Archive &ar) { ar(x, u, rho, v); }
};

std::ostream &operator<<(std::ostream &os, const cell_t &c) {
  return os << "cell_t{x=" << c.x << " u=" << c.u << " rho=" << c.rho
            << " v=" << c.v << "}";
}

auto cell_axpy(const cell_t &y, const cell_t &x, real_t alpha) {
  return cell_t{y.x, alpha * x.u + y.u, alpha * x.rho + y.rho,
                alpha * x.v + y.v};
}

auto cell_analytic(real_t t, real_t x) {
  // Standing wave: u(t,x) = cos(k t) sin(k x) with k = 2\pi
  auto k = 2 * M_PI / (parameters.xmax - parameters.xmin);
  auto u = std::cos(k * t) * std::sin(k * x);
  auto rho = -k * std::sin(k * t) * std::sin(k * x);
  auto v = k * std::cos(k * t) * std::cos(k * x);
  return cell_t{x, u, rho, v};
}

auto cell_init(real_t t, real_t x) { return cell_analytic(t, x); }

auto cell_error(const cell_t &c, real_t t) {
  auto a = cell_analytic(t, c.x);
  return cell_axpy(c, a, -1.0);
}

auto cell_norm(const cell_t &c) {
  return norm_t(c.u) + norm_t(c.rho) + norm_t(c.v);
}

auto cell_energy(const cell_t &c) {
  return parameters.dx * (0.5 * std::pow(c.rho, 2) + 0.5 * std::pow(c.v, 2));
}

// Note: These Dirichlet boundaries are unstable
auto cell_boundary_dirichlet(real_t t, int_t i) {
  return cell_analytic(t, i == 0 ? parameters.xmin - 0.5 * parameters.dx
                                 : parameters.xmax + 0.5 * parameters.dx);
}

auto cell_boundary_reflecting(const cell_t &c, int_t i) {
  return cell_t{i == 0 ? 2 * parameters.xmin - c.x : 2 * parameters.xmax - c.x,
                -c.u, -c.rho, c.v};
}

auto cell_rhs(const cell_t &c, size_t bdirs, const cell_t &bm,
              const cell_t &bp) {
  auto dx_1 = parameters.dx_1;
  auto u_rhs = c.rho;
  auto rho_rhs = (-0.5 * bm.v + 0.5 * bp.v) * dx_1;
  auto v_rhs = (-0.5 * bm.rho + 0.5 * bp.rho) * dx_1;
  return cell_t{0.0, u_rhs, rho_rhs, v_rhs};
}

// Grid

template <typename T>
using future_vector =
    adt::nested<qthread::shared_future<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T>
using proxy_vector =
    adt::nested<funhpc::proxy<adt::dummy>, std::vector<adt::dummy>, T>;
template <typename T> using proxy_tree = adt::tree<proxy_vector<adt::dummy>, T>;
template <typename T>
using proxy_tree_vector =
    adt::nested<proxy_tree<adt::dummy>, std::vector<adt::dummy>, T>;

// template <typename T> using storage_t = std::vector<T>;
// template <typename T> using storage_t = proxy_vector<T>;
// template <typename T> using storage_t = proxy_tree<T>;
template <typename T> using storage_t = proxy_tree_vector<T>;

struct grid_t {
  real_t time;
  storage_t<cell_t> cells;
};

std::ostream &operator<<(std::ostream &os, const grid_t &g) {
  os << "grid_t{\n"
     << "  time=" << g.time << "\n"
     << "  cells=" << fun::to_ostreamer(g.cells) << "\n"
     << "}\n";
  return os;
}

auto grid_axpy(const grid_t &y, const grid_t &x, real_t alpha) {
  return grid_t{alpha * x.time + y.time,
                fun::fmap2(cell_axpy, y.cells, x.cells, alpha)};
}

auto grid_init(real_t t) {
  return grid_t{t, fun::iotaMap<storage_t<adt::dummy>>([t](int_t i) {
    real_t x = parameters.xmin + parameters.dx * (real_t(i) + 0.5);
    return cell_init(t, x);
  }, parameters.ncells)};
}

auto grid_error(const grid_t &g) {
  return grid_t{g.time, fun::fmap(cell_error, g.cells, g.time)};
}

auto grid_norm(const grid_t &g) {
  return fun::foldMap(cell_norm, norm_add, norm_zero(), g.cells);
}

auto grid_energy(const grid_t &g) {
  return fun::foldMap(cell_energy, std::plus<real_t>(), 0.0, g.cells);
}

auto grid_boundary(const grid_t &g, int_t i) {
  // return cell_boundary_dirichlet(g.time, i);
  return cell_boundary_reflecting(
      i == 0 ? fun::head(g.cells) : fun::last(g.cells), i);
}

auto cell_get_face(const cell_t &c, int_t i) { return c; }
auto grid_rhs(const grid_t &g) {
  return grid_t{1.0,
                fun::fmapStencil(CXX_FUNOBJ(cell_rhs),
                                 CXX_FUNOBJ(cell_get_face), g.cells, 0b11,
                                 grid_boundary(g, 0), grid_boundary(g, 1))};
}

// State

struct state_t {
  int_t iter;
  grid_t state;
  grid_t error;
  qthread::shared_future<norm_t> fnorm;
  qthread::shared_future<real_t> fenergy;
  grid_t rhs;
  state_t(int_t iter, const grid_t &state)
      : iter(iter), state(state), error(grid_error(state)),
        fnorm(qthread::async(grid_norm, error)),
        fenergy(qthread::async(grid_energy, state)), rhs(grid_rhs(state)) {}
};

auto euler(const state_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  return grid_axpy(s0, r0, parameters.dt);
}

auto rk2(const state_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  auto s1 = grid_axpy(s0, r0, 0.5 * parameters.dt);
  auto r1 = grid_rhs(s1);
  return grid_axpy(s0, r1, parameters.dt);
}

// Output

int info_output(int token, const state_t &s) {
  if (s.iter % parameters.outinfo_every == 0 || s.iter == parameters.nsteps) {
    std::cout << "[" << s.iter << "] " << s.state.time << ": "
              << s.fnorm.get().norm2() << " " << s.fenergy.get() << "\n";
  }
  return token;
}

struct cell_to_ostreamer {
  real_t t;
  template <typename Archive> void serialize(Archive &ar) { ar(t); }
  auto operator()(const cell_t &cs, const cell_t &ce) const {
    fun::ostreamer ostr;
    ostr << t << "\t" << cs.x << "\t" << cs.u << "\t" << cs.rho << "\t" << cs.v
         << "\t" << ce.u << "\t" << ce.rho << "\t" << ce.v << "\t"
         << cell_energy(cs) << "\n";
    return ostr;
  };
};

int file_output(int token, const state_t &s) {
  if (s.iter % parameters.outfile_every == 0 || s.iter == parameters.nsteps) {
    std::fstream fs;
    auto mode = s.iter == 0 ? std::ios::in | std::ios::out | std::ios::trunc
                            : std::ios::in | std::ios::out | std::ios::ate;
    fs.open(parameters.outfile_name, mode);
    fs << fun::foldMap2(cell_to_ostreamer{s.state.time},
                        fun::combine_ostreamers(), fun::ostreamer(),
                        s.state.cells, s.error.cells) << "\n";
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
  parameters.setup();
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
