#include <adt/array.hpp>
#include <adt/grid.hpp>
#include <cxx/apply.hpp>
#include <cxx/tuple.hpp>
#include <cxx/utility.hpp>
#include <fun/array.hpp>
#include <fun/proxy.hpp>
#include <fun/shared_future.hpp>
#include <fun/topology.hpp>
#include <fun/vector.hpp>
#include <funhpc/main.hpp>
#include <funhpc/rexec.hpp>

#include <fun/grid.hpp>
#include <fun/nested.hpp>
#include <fun/tree.hpp>

#include <fun/fun.hpp>

#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

// Types

typedef std::ptrdiff_t int_t;
typedef double real_t;

// Dimension

constexpr int dim = 1;
typedef std::array<int_t, dim> vint_t;
typedef std::array<real_t, dim> vreal_t;

const vint_t vizero = adt::array_zero<ptrdiff_t, dim>();
const vint_t vione = adt::array_fill<ptrdiff_t, dim>(1);
inline vint_t vidir(int_t d) { return adt::array_dir<int_t, dim>(d); }

const vreal_t vzero = adt::array_zero<real_t, dim>();
const vreal_t vone = adt::array_fill<real_t, dim>(1.0);
inline vreal_t vdir(int_t d) { return adt::array_dir<real_t, dim>(d); }

// Parameters

struct parameters_t {
  vint_t ncells;
  const vreal_t xmin = vone * 0.0;
  const vreal_t xmax = vone * 1.0;
  vreal_t dx() const { return (xmax - xmin) / ncells; }

  int_t nsteps;
  const real_t tmin = 0.0;
  const int_t icfl = dim == 1 ? 1 : 2;
  real_t dt() const { return adt::maxval(dx()) / icfl; }

  int_t outinfo_every;
  int_t outfile_every;
  std::string outfile_name;
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
    min = std::fmin(min, other.min);
    max = std::fmax(max, other.max);
    max_abs = std::fmax(max_abs, other.max_abs);
    sum += other.sum;
    sum2 += other.sum2;
    sum_abs += other.sum_abs;
    return *this;
  }
  norm_t operator+(const norm_t &other) const { return norm_t(*this) += other; }

  static norm_t unit(real_t x) { return norm_t(x); }
  static norm_t plus(const norm_t &x, const norm_t &y) { return x + y; }
  static norm_t zero() { return norm_t(); }

  norm_t(vreal_t xs) : norm_t(fun::foldMap(unit, plus, zero(), xs)) {}

  real_t avg() const { return sum / count; }
  real_t norm1() const { return sum_abs / count; }
  real_t norm2() const { return std::sqrt(sum2 / count); }
  real_t norm_inf() const { return max_abs; }
};

// Cell

struct cell_t {
  vreal_t x;
  real_t u, rho;
  vreal_t v;
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

auto cell_analytic(real_t t, vreal_t x) {
  // Standing wave: u(t,x) = cos(k t) sin(k x) with k = 2\pi
  real_t(&sind)(real_t) = std::sin;
  auto k = 2 * M_PI / (parameters.xmax - parameters.xmin);
  auto w = std::sqrt(adt::sum(k * k));
  auto u = std::cos(w * t) * adt::prod(fun::fmap(sind, k * x));
  auto rho = -w * std::sin(w * t) * adt::prod(fun::fmap(sind, k * x));
  vreal_t v;
  for (int_t i = 0; i < dim; ++i) {
    v[i] = std::cos(w * t);
    for (int_t j = 0; j < dim; ++j)
      v[i] *= j == i ? k[j] * std::cos(k[j] * x[j]) : std::sin(k[j] * x[j]);
  }
  return cell_t{x, u, rho, v};
}

auto cell_init(real_t t, vreal_t x) { return cell_analytic(t, x); }

auto cell_error(const cell_t &c, real_t t) {
  auto a = cell_analytic(t, c.x);
  return cell_axpy(c, a, -1.0);
}

auto cell_norm(const cell_t &c) {
  return norm_t(c.u) + norm_t(c.rho) + norm_t(c.v);
}

auto cell_energy(const cell_t &c) {
  return adt::prod(parameters.dx()) * 0.5 *
         (c.rho * c.rho + adt::sum(c.v * c.v));
}

// Note: These Dirichlet boundaries are unstable
auto cell_boundary_dirichlet(const cell_t &c, real_t t, int_t i) {
  auto f = i % 2, d = i / 2;
  auto bdir = (!f ? -1 : +1) * vdir(d);
  auto xbnd = c.x + bdir * parameters.dx();
  return cell_analytic(t, xbnd);
}

auto cell_boundary_reflecting(const cell_t &c, int_t i) {
  auto f = i % 2, d = i / 2;
  auto bdir = (!f ? -1 : +1) * vdir(d);
  auto xbnd = c.x + bdir * parameters.dx();
  return cell_t{xbnd, -c.u, -c.rho, adt::update(-c.v, d, c.v[d])};
}

auto cell_get_face(const cell_t &c, int_t i) { return c; }

template <typename... Bnds> auto cell_rhs(const cell_t &c, Bnds &&... bnds) {
  static_assert(sizeof...(Bnds) == 2 * dim, "");
  static_assert(cxx::all_of_type<
                    std::is_same<std::decay_t<Bnds>, cell_t>::value...>::value,
                "");
  auto bs = std::forward_as_tuple(bnds...);
  auto bm = cxx::to_array(cxx::tuple_section<0, dim>(bs));
  auto bp = cxx::to_array(cxx::tuple_section<dim, dim>(bs));

  auto dx = parameters.dx();
  auto u_rhs = c.rho;
  real_t rho_rhs = 0.0;
  for (int_t j = 0; j < dim; ++j)
    rho_rhs += (-0.5 * bm[j].v[j] + 0.5 * bp[j].v[j]) / dx[j];
  vreal_t v_rhs;
  for (int_t i = 0; i < dim; ++i)
    v_rhs[i] = (-0.5 * bm[i].rho + 0.5 * bp[i].rho) / dx[i];
  return cell_t{vzero, u_rhs, rho_rhs, v_rhs};
}

template <typename... Bnds>
auto get_cell_rhs(const std::tuple<Bnds...> &)
    -> cell_t (*)(const cell_t &, const Bnds &...);
typedef decltype(
    get_cell_rhs((std::declval<cxx::ntuple<cell_t, 2 * dim>>()))) cell_rhs_t;

// Grid

template <typename T> using std_vector = std::vector<T>;
template <typename T> using vector_grid = adt::grid<std_vector, T, dim>;
// template <typename T>
// using proxy_grid = adt::nested<funhpc::proxy, std_vector, T>;
// template <typename T> using proxy_tree = adt::tree<proxy_grid, T>;

struct grid_t {
#warning "TODO: use tree"
#warning "TODO: use grid"
  template <typename T> using storage_t = vector_grid<T>;

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
  return grid_t{t, fun::iotaMap<grid_t::storage_t>([t](vint_t i) {
    vreal_t x = parameters.xmin +
                parameters.dx() *
                    (fun::fmap([](int_t i) { return real_t(i); }, i) + 0.5);
    return cell_init(t, x);
  }, parameters.ncells)};
}

auto grid_error(const grid_t &g) {
  return grid_t{g.time, fun::fmap(cell_error, g.cells, g.time)};
}

auto grid_norm(const grid_t &g) {
  return fun::foldMap(cell_norm, norm_t::plus, norm_t::zero(), g.cells);
}

auto grid_energy(const grid_t &g) {
  return fun::foldMap(cell_energy, std::plus<real_t>(), 0.0, g.cells);
}

auto grid_boundary(const grid_t &g, int_t i) {
  // return fun::boundaryMap(cell_boundary_dirichlet, g.cells, i, g.time, i);
  return fun::boundaryMap(cell_boundary_reflecting, g.cells, i, i);
}

template <typename F, typename G, typename TS, typename BS,
          std::size_t... Indices, typename... Args>
auto wrap_fmapStencil(F &&f, G &&g, TS &&xs, BS &&bs,
                      std::index_sequence<Indices...>, Args &&... args) {
  return fun::fmapStencil(
      std::forward<F>(f), std::forward<G>(g), std::forward<TS>(xs),
      std::get<Indices>(std::forward<BS>(bs))..., std::forward<Args>(args)...);
}

auto grid_rhs(const grid_t &g) {
#warning "TODO: Introduce a typedef in grid, or better grid_traits"
  std::array<adt::grid<std_vector, cell_t, dim - 1>, dim> bms, bps;
  for (std::ptrdiff_t d = 0; d < dim; ++d) {
    bms[d] = grid_boundary(g, 2 * d + 0);
    bps[d] = grid_boundary(g, 2 * d + 1);
  }
  auto bs = std::tuple_cat(std::move(bms), std::move(bps));

  return grid_t{1.0, wrap_fmapStencil((cell_rhs_t)cell_rhs, cell_get_face,
                                      g.cells, std::move(bs),
                                      std::make_index_sequence<2 * dim>())};
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
  std::cout << "Wave3d\n";
  parameters.ncells = vione * 100;
  parameters.nsteps = adt::maxval(parameters.ncells) * parameters.icfl;
  parameters.outinfo_every = parameters.nsteps / 10;
  parameters.outfile_every = parameters.nsteps / 20;
  parameters.outfile_name = "wave3d.tsv";
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
