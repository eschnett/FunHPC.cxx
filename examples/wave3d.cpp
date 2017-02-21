#include <adt/dummy.hpp>
#include <adt/index.hpp>
#include <cxx/apply.hpp>
#include <cxx/funobj.hpp>
#include <cxx/tuple.hpp>
#include <cxx/utility.hpp>
#include <fun/array.hpp>
#include <fun/fun_decl.hpp>
#include <fun/grid_decl.hpp>
#include <fun/maxarray.hpp>
#include <fun/nested_decl.hpp>
#include <fun/proxy.hpp>
#include <fun/shared_future.hpp>
#include <fun/shared_ptr.hpp>
#include <fun/tree_decl.hpp>
#include <fun/vector.hpp>
#include <funhpc/main.hpp>
#include <funhpc/rexec.hpp>

#include <fun/fun_impl.hpp>
#include <fun/grid_impl.hpp>
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
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

// Types

typedef std::ptrdiff_t int_t;
typedef double real_t;

// Dimension

constexpr int dim = 2;
typedef std::array<int_t, dim> vint_t;
typedef std::array<real_t, dim> vreal_t;

const vint_t vizero = adt::zero<vint_t>();
const vint_t vione = adt::one<vint_t>();
inline vint_t vidir(int_t d) { return adt::dir<vint_t>(d); }

const vreal_t vzero = adt::zero<vreal_t>();
const vreal_t vone = adt::one<vreal_t>();
inline vreal_t vdir(int_t d) { return adt::dir<vreal_t>(d); }

// Parameters

struct parameters_t {
  vint_t ncells;
  const vreal_t xmin = vone * 0.0;
  const vreal_t xmax = vone * 1.0;
  vreal_t dx;
  vreal_t dx_1;

  int_t nsteps;
  const real_t tmin = 0.0;
  const int_t icfl = dim == 1 ? 1 : 2;
  real_t dt;

  int_t outinfo_every;
  int_t outfile_every;
  std::string outfile_name;

  void setup() {
    dx = (xmax - xmin) / ncells;
    dx_1 = 1.0 / dx;
    dt = adt::maxval(dx) / icfl;
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
  real_t (&sind)(real_t) = std::sin;
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
  return adt::prod(parameters.dx) * 0.5 * (c.rho * c.rho + adt::sum(c.v * c.v));
}

// Note: These Dirichlet boundaries are unstable
auto cell_boundary_dirichlet(const cell_t &c, int_t i, real_t t) {
  auto f = i % 2, d = i / 2;
  auto bdir = (!f ? -1 : +1) * vdir(d);
  auto xbnd = c.x + bdir * parameters.dx;
  return cell_analytic(t, xbnd);
}

auto cell_boundary_reflecting(const cell_t &c, int_t i) {
  auto f = i % 2, d = i / 2;
  auto bdir = (!f ? -1 : +1) * vdir(d);
  auto xbnd = c.x + bdir * parameters.dx;
  return cell_t{xbnd, -c.u, -c.rho, adt::update(-c.v, d, c.v[d])};
}

auto cell_get_face(const cell_t &c, int_t i) { return c; }

template <typename... Bnds>
auto cell_rhs(const cell_t &c, std::size_t bdirs, Bnds &&... bnds) {
  static_assert(sizeof...(Bnds) == 2 * dim, "");
  static_assert(cxx::all_of_type<
                    std::is_same<std::decay_t<Bnds>, cell_t>::value...>::value,
                "");
  auto bs = std::forward_as_tuple(bnds...);
  auto bm = cxx::to_array(cxx::tuple_section<0, dim>(bs));
  auto bp = cxx::to_array(cxx::tuple_section<dim, dim>(bs));

  auto dx_1 = parameters.dx_1;
  auto u_rhs = c.rho;
  real_t rho_rhs = 0.0;
  for (int_t j = 0; j < dim; ++j)
    rho_rhs += (-0.5 * bm[j].v[j] + 0.5 * bp[j].v[j]) * dx_1[j];
  vreal_t v_rhs;
  for (int_t i = 0; i < dim; ++i)
    v_rhs[i] = (-0.5 * bm[i].rho + 0.5 * bp[i].rho) * dx_1[i];
  return cell_t{vzero, u_rhs, rho_rhs, v_rhs};
}

template <typename... Bnds>
auto get_cell_rhs(const std::tuple<Bnds...> &)
    -> cell_t (*)(const cell_t &, std::size_t, const Bnds &...);
typedef decltype(
    get_cell_rhs((std::declval<cxx::ntuple<cell_t, 2 * dim>>()))) cell_rhs_t;

// Grid

constexpr std::size_t max_size = 16;

template <typename T>
using maxarray_grid = adt::grid<adt::maxarray<adt::dummy, max_size>, T, dim>;

template <typename T>
using shared_grid =
    adt::nested<std::shared_ptr<adt::dummy>, maxarray_grid<adt::dummy>, T>;
template <typename T>
using future_grid = adt::nested<qthread::shared_future<adt::dummy>,
                                maxarray_grid<adt::dummy>, T>;
template <typename T>
using proxy_grid =
    adt::nested<funhpc::proxy<adt::dummy>, maxarray_grid<adt::dummy>, T>;

template <typename T> using shared_tree = adt::tree<shared_grid<adt::dummy>, T>;

template <typename T> using future_tree = adt::tree<future_grid<adt::dummy>, T>;

template <typename T> using proxy_tree = adt::tree<proxy_grid<adt::dummy>, T>;

template <typename T>
using shared_grid_tree =
    adt::nested<adt::tree<shared_grid<adt::dummy>, adt::dummy>,
                shared_grid<adt::dummy>, T>;

template <typename T>
using future_grid_tree =
    adt::nested<adt::tree<future_grid<adt::dummy>, adt::dummy>,
                future_grid<adt::dummy>, T>;

template <typename T>
using proxy_grid_tree =
    adt::nested<adt::tree<proxy_grid<adt::dummy>, adt::dummy>,
                proxy_grid<adt::dummy>, T>;

// TOOD: Correct handling of boundaries (?) for adt::nested to make
// the other storage types work
template <typename T> using storage_t = maxarray_grid<T>;

// template <typename T> using storage_t = shared_grid<T>;
// template <typename T> using storage_t = future_grid<T>;
// template <typename T> using storage_t = proxy_grid<T>;

// template <typename T> using storage_t = shared_tree<T>;
// template <typename T> using storage_t = future_tree<T>;
// template <typename T> using storage_t = proxy_tree<T>;

// template <typename T> using storage_t = shared_grid_tree<T>;
// template <typename T> using storage_t = future_grid_tree<T>;
// template <typename T> using storage_t = proxy_grid_tree<T>;

typedef typename fun::fun_traits<storage_t<adt::dummy>>::boundary_dummy
    boundary_dummy;
template <typename T>
using boundary_t =
    typename fun::fun_traits<boundary_dummy>::template constructor<T>;

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
  return grid_t{
      t, fun::iotaMapMulti<storage_t<adt::dummy>>(
             [t](vint_t i) {
               vreal_t x =
                   parameters.xmin +
                   parameters.dx *
                       (fun::fmap([](int_t i) { return real_t(i); }, i) + 0.5);
               return cell_init(t, x);
             },
             adt::steprange_t<dim>(parameters.ncells))};
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
  // return fun::boundaryMap(cell_boundary_dirichlet, g.cells, i, g.time);
  return fun::boundaryMap(cell_boundary_reflecting, g.cells, i);
}

template <typename F, typename G, typename TS, typename BS,
          std::size_t... Indices, typename... Args>
auto wrap_fmapStencil(F &&f, G &&g, TS &&xs, BS &&bs,
                      std::index_sequence<Indices...>, Args &&... args) {
  std::size_t bmask = ~0;
  return fun::fmapStencilMulti<dim>(
      std::forward<F>(f), std::forward<G>(g), std::forward<TS>(xs), bmask,
      std::get<Indices>(std::forward<BS>(bs))..., std::forward<Args>(args)...);
}

auto grid_rhs(const grid_t &g) {
  std::array<boundary_t<cell_t>, dim> bms, bps;
  for (std::ptrdiff_t d = 0; d < dim; ++d) {
    bms[d] = grid_boundary(g, 2 * d + 0);
    bps[d] = grid_boundary(g, 2 * d + 1);
  }
  auto bs = std::tuple_cat(std::move(bms), std::move(bps));
  std::size_t bmask = ~0;

  // return grid_t{1.0,
  //               wrap_fmapStencil( // CXX_FUNOBJ((cell_rhs_t)cell_rhs),
  //                   // CXX_FUNOBJ(cell_rhs<const cell_t &, const cell_t &>),
  //                   CXX_FUNOBJ(cell_rhs<const cell_t &, const cell_t &,
  //                                       const cell_t &, const cell_t &>),
  //                   CXX_FUNOBJ(cell_get_face), g.cells, std::move(bs),
  //                   std::make_index_sequence<2 * dim>())};
  // static_assert(dim == 1, "");
  // return grid_t{1.0, fun::fmapStencilMulti<dim>(
  //                        CXX_FUNOBJ(cell_rhs<const cell_t &, const cell_t
  //                        &>),
  //                        CXX_FUNOBJ(cell_get_face), g.cells, bmask,
  //                        std::get<0>(bs), std::get<1>(bs))};
  static_assert(dim == 2, "");
  return grid_t{1.0,
                fun::fmapStencilMulti<dim>(
                    CXX_FUNOBJ(cell_rhs<const cell_t &, const cell_t &,
                                        const cell_t &, const cell_t &>),
                    CXX_FUNOBJ(cell_get_face), g.cells, bmask, std::get<0>(bs),
                    std::get<1>(bs), std::get<2>(bs), std::get<3>(bs))};
}

// State

struct schedule_t {
  int_t iter;
  grid_t state;
  grid_t error;
  qthread::shared_future<norm_t> fnorm;
  qthread::shared_future<real_t> fenergy;
  grid_t rhs;
  schedule_t(int_t iter, const grid_t &state)
      : iter(iter), state(state), error(grid_error(state)),
        fnorm(qthread::async(grid_norm, error)),
        fenergy(qthread::async(grid_energy, state)), rhs(grid_rhs(state)) {}
};

auto euler(const schedule_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  return grid_axpy(s0, r0, parameters.dt);
}

auto rk2(const schedule_t &s) {
  const grid_t &s0 = s.state;
  const grid_t &r0 = s.rhs;
  auto s1 = grid_axpy(s0, r0, 0.5 * parameters.dt);
  auto r1 = grid_rhs(s1);
  return grid_axpy(s0, r1, parameters.dt);
}

// Output

// TODO: Accept and return a shared_future<int>, and call fmap only if
// there should be output during this iteration. Question: Does this
// work with checking s.iter?
int info_output(int token, const schedule_t &s) {
  if ((parameters.outinfo_every == 0 && s.iter == 0) ||
      (parameters.outinfo_every > 0 &&
       s.iter % parameters.outinfo_every == 0) ||
      s.iter == parameters.nsteps) {
    std::cout << "[" << s.iter << "] " << s.state.time << ": "
              << s.fnorm.get().norm2() << " " << s.fenergy.get() << "\n"
              << std::flush;
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

int file_output(int token, const schedule_t &s) {
  if ((parameters.outfile_every == 0 && s.iter == 0) ||
      (parameters.outfile_every > 0 &&
       s.iter % parameters.outfile_every == 0) ||
      s.iter == parameters.nsteps) {
    std::fstream fs;
    auto mode = s.iter == 0 ? std::ios::in | std::ios::out | std::ios::trunc
                            : std::ios::in | std::ios::out | std::ios::ate;
    fs.open(parameters.outfile_name, mode);
    fs << fun::foldMap2(cell_to_ostreamer{s.state.time},
                        fun::combine_ostreamers(), fun::ostreamer(),
                        s.state.cells, s.error.cells)
       << "\n";
    fs.close();
  }
  return token;
}

template <typename T> class delay_window {
  static constexpr std::size_t window_every = 10;
  static constexpr std::size_t window_size = 1;
  std::size_t counter = 0;
  std::queue<T> events;

public:
  void delayed_wait(const T &event) {
    counter = (counter + 1) % window_every;
    if (counter != 0)
      return;
    events.push(event);
    if (events.size() <= window_size)
      return;
    events.front().wait();
    events.pop();
  }
};

// Driver

int funhpc_main(int argc, char **argv) {
  std::cout << "Wave3d\n";
  parameters.ncells = vione * 100;
  parameters.nsteps = adt::maxval(parameters.ncells) * parameters.icfl;
  parameters.outinfo_every = parameters.nsteps / 10;
  parameters.outfile_every = -1; // TODO parameters.nsteps / 20;
  parameters.outfile_name = "wave3d.tsv";
  parameters.setup();
  qthread::shared_future<int> info_token = qthread::make_ready_future(0);
  qthread::shared_future<int> file_token = qthread::make_ready_future(0);
  auto s = std::make_shared<schedule_t>(0, grid_init(parameters.tmin));
  info_token = fun::fmap(info_output, info_token, *s);
  file_token = fun::fmap(file_output, file_token, *s);
  delay_window<qthread::shared_future<int>> info_tokens, file_tokens;
  info_tokens.delayed_wait(info_token);
  file_tokens.delayed_wait(file_token);
  while (s->iter < parameters.nsteps) {
    s = std::make_shared<schedule_t>(s->iter + 1, rk2(*s));
    info_token = fun::fmap(info_output, info_token, *s);
    file_token = fun::fmap(file_output, file_token, *s);
    info_tokens.delayed_wait(info_token);
    file_tokens.delayed_wait(file_token);
  }
  info_token.wait();
  file_token.wait();
  std::cout << "Done.\n";
  return 0;
}
