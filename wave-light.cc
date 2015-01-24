/*
  The scalar wave equation "light": Multi-threaded via standard C++14
  mechanisms, without external dependencies.
*/

/*
  Section 1: Introduction

  This is an attempt at literal programming in C++. That means that
  text -- in this case a tutorial -- is embedded into the code. To
  keep things simple, everying comes as a single file, to read from
  top to bottom.

  This wave equation solver is implemented in a functional (aka
  declarative) style. That means that objects, once created, are not
  modified. Consequently, all inputs to a routine need to be passed in
  as arguments; objects cannot have fields that are set from the
  outside, or buffers where they receive data. This style takes a bit
  getting-used-to, but has several significant advantages. Among these
  are (1) no deadlocks when using multi-threading, (2) objects can
  easily be passed between threads, and (3) in a distributed
  environment, objects can be copied to remote locations without
  affecting semantics or correctness.
*/

////////////////////////////////////////////////////////////////////////////////

/*
  Section 2: Boilerplate

  Let's begin with the usual boilerplate, #include statements.
*/

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////////////

/*
  Section 3: Utilities

  These utilities should really be built into C++. Nothing to see
  here, move on.
*/

// Various utility routines

// Integer division, rounding down
template <typename T> T div_floor(T x, T y) {
  assert(y > 0);
  T r = (x >= 0 ? x : x - y + 1) / y;
  assert(r * y <= x && (r + 1) * y > x);
  return r;
}

// Integer division, rounding up
template <typename T> T div_ceil(T x, T y) {
  assert(y > 0);
  T r = (x > 0 ? x + y - 1 : x) / y;
  assert(r * y >= x && (r - 1) * y < x);
  return r;
}

// Create a ready future
template <typename T> future<std::decay_t<T> > make_ready_future(T &&value) {
  promise<std::decay_t<T> > p;
  p.set_value(std::forward<T>(value));
  return p.get_future();
}

inline future<void> make_ready_future() {
  promise<void> p;
  p.set_value();
  return p.get_future();
}

////////////////////////////////////////////////////////////////////////////////

/*
  Section 4: Run-time parameters

  Every simulation code needs run-time parameters. Here I use
  compile-time parameters for simplicity; you would modify them at
  will, and then recompile.

  For this example, a simple namespace would suffice. For a
  distributed code, one may need to send parameter settings to other
  nodes, so I use a struct instead.

  I explain the parameters' meanings near the places where they are
  used.
*/

// Global definitions, a poor man's parameter file

struct defs_t {
  // const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t rho = 64; // resolution scale
  const ptrdiff_t ncells_per_grid = 4;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.5;
  const double tmin = 0.0;
  const double tmax = 1.0;
  const ptrdiff_t nsteps = -1;
  // const ptrdiff_t nsteps = 8;

  ptrdiff_t ncells;
  double dx;
  double dt;

  const ptrdiff_t wait_every = 0;
  const ptrdiff_t info_every = 8;
  // const ptrdiff_t info_every = 0;
  const ptrdiff_t file_every = 0;
  // const ptrdiff_t file_every = -1;
  defs_t(int nthreads)
      : ncells(rho * ncells_per_grid * nthreads), dx((xmax - xmin) / ncells),
        dt(cfl * dx) {}
};
shared_ptr<defs_t> defs;

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

/*
  Section 5: Norms

  We'll need to calculate norms of the state vector. To simplify
  things, we define a class norm_t for this. norm_t acts as
  accumulator while calculating norms. norm_t has a neutral element
  (created by the default constructor), a munit constructor (from a
  double value), and one can combine two norm_t objects, for which we
  overload operator+.

  Given a norm_t object, one can obtain the number of values which
  contributed, their sum and the sum of their squares, as well as
  average and L2-norm.

  Note that there is no operator+= that would modify a norm object.
  Instead, one uses operator+ to create a new norm object. This is at
  the heart of what defines functional programming. We assume that
  this class and its member functions are so simple that the compiler
  will inline everything, and there will be no performance overhead
  coming from our functional programming style.

  This class is generic and has nothing to do with the scalar wave
  equation.
*/

// A norm

struct norm_t {
  double sum, sum2, count;

  norm_t() : sum(0.0), sum2(0.0), count(0.0) {}
  norm_t(double val) : sum(val), sum2(pow(val, 2)), count(1.0) {}
  norm_t(const norm_t &x, const norm_t &y)
      : sum(x.sum + y.sum), sum2(x.sum2 + y.sum2), count(x.count + y.count) {}
  auto avg() const { return sum / count; }
  auto norm2() const { return sqrt(sum2 / count); }
};
inline auto operator+(const norm_t &x, const norm_t &y) { return norm_t(x, y); }

////////////////////////////////////////////////////////////////////////////////

/*
  Section 6: Cells

  A cell holds the data we need for a single grid point. On a techical
  level, this defines the tangent space we use. We separate the
  operations for each grid point from the container that holds grids
  points (which will be defined below) to structure the code. As for
  the norm_t class above, we assume that the compiler will inline all
  the operations on cells, so that this will not lead to an overhead.
  (I checked this, and this seems to be true.)

  A cell holds its coordinate x, the scalar wave potential u, and time
  and space derivatives rho and v.

  The output routine is straightforward.

  axpy performs a linear combination of two cells. This is needed in
  several places, e.g. for time integration, or to calculate the error
  (the difference to the analytic solution). Instead of a single
  routine axpy, we could have overloaded the usual operators + - * /
  to achieve the same effect, and probably in a more C++-like fashion.
  However, this would have required us to write (at least) eight
  routines, and we don't want to increase the code size too much for
  this tutorial.

  (For the curious: The eight routines mentioned above are for unary +
  and -, binary + and -, as well as * and / where either the first or
  the second argument is a scalar; all in all eight routines. Also,
  C++ requires that at least msome of these be defined outside the
  class.)

  You will note that the axpy "routine" is written as constructor,
  with a dummy tupe axpy. This dummy type is empty. I experimented
  with various ways of writing routines in C++ that return a new
  object. I found that using a constructor looks best and is simplest.
  The disadvantage is that all constructures for a class in C++ have
  the same name, the name of the class. There is a priori no way to
  specify "construct this object via a linear combination", or
  "construct this object by applying a boundary condition". Hence I
  pass in an additional dummy argument that specifies which operation
  to perform. This dummy object is empty, and (I checked!) completely
  disappears at run time; there is no overhead. Thus, a function call
  "cell_t(cell_t::axpy(), a, x, y)" can be read as a fancy way of
  writing "cell_t::axpy(a, x, y)", where "cell_t::axpy" would be a
  static class function.

  Writing routines as constructors has nothing directly to do with a
  functional programming style; if you dislike this style, then it is
  easy to implement axpy as a static function, or as a member function
  where e.g. "y" is passed as "this*".

  The norm routine looks as we expect from the norm_t class above;
  each element of the state vector (except the coordinate) is
  transformed into a norm, and these norms are then combined (via
  operator+) to a single norm that is returned.

  There is a routine to evaluate an analytic solution. This routine is
  used for initial and boundary conditions -- nothing special here
  that you wouldn't expect in a toy code.

  The RHS routine shows again the functional programming style. Since
  we are separating the pointwise operations from the container
  holding our grid, we can't directly access neighbouring grid points.
  The neighbourhood information is only known to the container. Thus,
  all inputs need to be passed in, which includes all stencil points
  that we need to evaluate the RHS.

  Here, we pass the neighbouring points as cell_t; in a finite element
  code, we would probably pass only the boundaries of the neighbouring
  elements, not the complete elements.
*/

// A cell -- a container holding data, no intelligence here

struct cell_t {
  static constexpr auto nan = numeric_limits<double>::quiet_NaN();

  double x;
  double u;
  double rho;
  double v;

  // For safety
  cell_t() : x(nan), u(nan), rho(nan), v(nan) {}

  // Output
  auto output(ostream &os) const -> ostream & {
    return os << "cell: x=" << x << " u=" << u << " rho=" << rho << " v=" << v;
  }

  // Linear combination
  struct axpy {};
  cell_t(axpy, double a, const cell_t &x, const cell_t &y) {
    this->x = a * x.x + y.x;
    u = a * x.u + y.u;
    rho = a * x.rho + y.rho;
    v = a * x.v + y.v;
  }

  // Norm
  auto norm() const { return norm_t(u) + norm_t(rho) + norm_t(v); }

  // Analytic solution
  struct analytic {};
  cell_t(analytic, double t, double x) {
    this->x = x;
    u = sin(2 * M_PI * t) * sin(2 * M_PI * x);
    rho = 2 * M_PI * cos(2 * M_PI * t) * sin(2 * M_PI * x);
    v = 2 * M_PI * sin(2 * M_PI * t) * cos(2 * M_PI * x);
  }

  // Initial condition
  struct initial {};
  cell_t(initial, double t, double x) : cell_t(analytic(), t, x) {}

  // Boundary condition
  struct boundary {};
  cell_t(boundary, double t, double x) : cell_t(analytic(), t, x) {}

  // Error
  struct error {};
  cell_t(error, const cell_t &c, double t)
      : cell_t(axpy(), -1.0, cell_t(analytic(), t, c.x), c) {}

  // RHS
  struct rhs {};
  cell_t(rhs, const cell_t &c, const cell_t &cm, const cell_t &cp) {
    x = 0.0; // dx/dt
    u = c.rho;
    rho = (cp.v - cm.v) / (2 * defs->dx);
    v = (cp.rho - cm.rho) / (2 * defs->dx);
  }
};

// Output
auto operator<<(ostream &os, const cell_t &c) -> ostream & {
  return c.output(os);
}

////////////////////////////////////////////////////////////////////////////////

/*
  Section 7: Grids (holding multiple grid points)

  Above we defined the operations on our state vector. Here we define
  a container that holds multiple grid cells (points, elements). As
  alluded above, we expect that all operations on cells will be
  inlined into the respective routines defined for a grid. My mental
  model for a grid is "a reasonably small set of grid points with
  highly optimised oeprations". For example, we may want to pass a
  grid to a CUDA thread, or execute its operations on a Xeon Phi
  thread. There will be no parallelism inside a grid -- to mmake things
  parallel, we will use multiple grids, one per thread.

  The grid_t class definition is then straightforward. A grid knows
  which index rangs it represents (imin, imax), and there is a helper
  routine to convert indices to coordinates. The "get" routine expects
  a global index and returns the respective grid cell.

  Each of the cell_t functions above has an equivalent function here,
  which performs the same action, but on the whole grid instead of on
  a single cell.

  There output routine is straightforward; it simply loops over all
  grid cells.

  The linear combination routine "axpy" shows, as above, our
  functional programming style. We initialise as many class fields as
  possible with the C++ constructor notation, including allocating the
  grid cells. However, for the grid cells themselves we need a for
  loop, so this is implemented in the body of the routine. Within this
  for loop we access the respective grid cells ("x.cells[i]",
  "y.cells[i]"), and then call the cell's axpy routine. As mentioned
  before, this call will be inlined.

  Note that this allocate a new vector of cells. At first sight, this
  may seem expensive, as almost every call to grid_t will allocate
  memory. However, this is actually not that bad for two reasons:

  (1) In a multi-threaded program, it is very difficult to ensure that
  one can re-use an existing array instead of allocating a new one.
  Re-using existing arrays requires a significant design complexity,
  is difficult to get correct, and if one has to wait for an old array
  to become available, may reduce available parallelism.

  (2) Here we allocate vectors with the standard allocator. A standard
  means to improve efficiency would be to use a custom allocator. This
  custom allocator could retain a large number of pre-allocated
  vectors of specific sizes, and could hand out those instead of
  allocating new vectors. In fact, using a custom allocator is very
  similar to re-using existing vectors, with the main difference that
  the complexity is hidden inside the allocator, which can on the fly
  decide whether the hand out an existing (unused) vector or whether
  to allocate a new one.

  Overall, I want to argue that the functional programming style leads
  to a much cleaner and simpler design, where it is a priori not clear
  at all whether performance will be reduced. The code here is not
  optimised for performance -- but it could be, and this would not
  require a complete re-design.

  The functions to calculate the norm, initial condition, and boundary
  condition are straightforward to understeand.

  The most complex routine here is probably the RHS routine. Its
  complexity comes mostly from the fact that it contains repeated
  code, making it longer than it should be. (I'll discuss ways to
  reduce this code duplication below.)

  As input, the RHS function expects a grid, as well as the boundary
  cells from the lower and upper neighbour grid. As for cell_t above,
  a grid does not know where in the domain topology it lives -- this
  information is only known higher up, and to evaluate the RHS, one
  needs to pass in the boundary information. This API is probably the
  single point where a functional program differs from an imperative
  one. I thus want to spend a bit of time discussing these
  differences.

  In an imperative program, the RHS routine would likely be
  "self-sufficient" in the sense that a grid would "know" its
  neighbours. While this may simplify calling the RHS function, it
  greatly complicates setting up the grids, and teaching each grid
  about its neighbours. This then complicates load balancing -- if one
  moves a grid to a different location, one has to teach each
  neighbour that it has moved, and if the neighbours may also be
  moving at the same time, then load balancing turns into a global,
  stop-the-world operation.

  In an imperative approach, where each grid knows about (holds
  pointers to) its neighbours, the graph connecting grids has cycles.
  In a functional approach, this graph is acyclic, making it much
  easier to understand.

  For example, to re-balance the load, one would simply allocate a
  newly calculated grid on a new process. That's it -- no other grids
  need to be taught about this. Also, since objects are immutable
  after they have been created, one can also simply copy objects to
  other locations, and it does not matter how many copies there are.
  For example, one could optimistically mmake ten copies of a grid at
  ask ten different compute nodes to evaluate the RHS, then using the
  first result that is produced. In a functional design, this is
  automatically correct and does not require any additional management
  overhead. In an imperative design, duplicating a grid would need to
  be managed carefully to ensure results remain consistent.

  Let us next discuss how to potentially simplify the routines in this
  grid_t class. You will have notices that most of the routines look
  very similar -- the routines mostly don't do anything, they only
  loop over cells and then ask the cell_t class to perform the actual
  operation. I identify four different loop patterns. (I list in
  quotes the names these loop patterns have in Lisp.)

  (1) Initialization ("iota"): In the function "initial", we loop from
  imin to imax, and pass the coordinate for the respective index to
  the cell initialization routine.

  (2) Simple traversal ("map"): In the function "error", we loop over
  each cell of an input grid, and create and output grid by applying
  the cell's eror function. In the function "axpy" we do the same,
  except there are two input grids.

  (3) Reduction ("fold"): In the function "norm", we initialise an
  accumulator, and then loop over all cells while modifying the
  accumulator. In the end, the accumulator holds the final result.

  (4) Stencilled traversal: This is technically a combination of a map
  and a fold operation, with a fold operation that is very efficient
  since it only involved neighbouring grids (via their boundaries).
  This pattern does not exist in Lisp.

  The way in which one loops over the elements of a grid depends on
  the data structure used to hold the cells. Here, we use a vector,
  for which a simple for loop is the "right" way. If we used a
  different data structure -- e.g. a multi-dimensional array, or a
  tree -- then we would need to use a different way for traversal,
  e.g. multiple nested for loops, or a recursive implementation. In
  general, it mmakes sense to define functions for these four traversal
  patterns for each data structure. Here I didn't do this to keep the
  code simpler. If one wants to use a distributed tree, then
  duplication the respective (probably recursive) traversal patterns
  for each routine would be quite complex. In this case, replacing
  e.g. the implementation of "error" which currently looks like this:

  for (size_t i = 0; i < cells.size(); ++i)
    cells[i] = cell_t(cell_t::error(), g.cells[i], t);

  with code such as:

  cells = g.cells.for_each_element
    ([t](auto c) { return cell_t(cell_t::error(), c, t); });

  would mmake sense. The lambda is a callback that is called from the
  traversal routine, passing in the current cell as variable "c". Note
  that the lambda evaluates exactly the same expression as the loop
  body did.
*/

// Each grid lives on a process

struct grid_t {
  ptrdiff_t imin, imax; // spatial indices
  auto x(ptrdiff_t i) const { return defs->xmin + (imin + i + 0.5) * defs->dx; }

  vector<cell_t> cells;

  auto get_boundary(bool face_upper) const {
    size_t s = cells.size();
    return !face_upper ? cells[0] : cells[s - 1];
  }

  // Output
  auto output(ostream &os) const -> ostream & {
    size_t s = cells.size();
    for (ptrdiff_t i = 0; i < s; ++i) {
      os << "   i=" << imin + i << " " << cells[i] << "\n";
    }
    return os;
  }

  // Linear combination
  struct axpy {};
  grid_t(axpy, double a, const grid_t &x, const grid_t &y)
      : imin(x.imin), imax(x.imax), cells(x.cells.size()) {
    assert(y.cells.size() == cells.size());
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::axpy(), a, x.cells[i], y.cells[i]);
  }

  // Norm
  auto norm() const -> norm_t {
    norm_t n;
    for (size_t i = 0; i < cells.size(); ++i)
      n = n + cells[i].norm();
    return n;
  }

  // Initial condition
  struct initial {};
  grid_t(initial, double t, ptrdiff_t imin)
      : imin(imin), imax(min(imin + defs->ncells_per_grid, defs->ncells)),
        cells(imax - imin) {
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::initial(), t, x(i));
  }

  // Error
  struct error {};
  grid_t(error, const grid_t &g, double t)
      : imin(g.imin), imax(g.imax), cells(g.cells.size()) {
    for (size_t i = 0; i < cells.size(); ++i)
      cells[i] = cell_t(cell_t::error(), g.cells[i], t);
  }

  // RHS
  struct rhs {};
  grid_t(rhs, const grid_t &g, const cell_t &bm, const cell_t &bp)
      : imin(g.imin), imax(g.imax), cells(g.cells.size()) {
    size_t s = cells.size();
    if (s == 1) {
      cells[0] = cell_t(cell_t::rhs(), g.cells[0], bm, bp);
    } else if (s >= 2) {
      cells[0] = cell_t(cell_t::rhs(), g.cells[0], bm, g.cells[1]);
      for (size_t i = 1; i < s - 1; ++i)
        cells[i] =
            cell_t(cell_t::rhs(), g.cells[i], g.cells[i - 1], g.cells[i + 1]);
      cells[s - 1] = cell_t(cell_t::rhs(), g.cells[s - 1], g.cells[s - 2], bp);
    }
  }
};

// Output
auto operator<<(ostream &os, const grid_t &g) -> ostream & {
  return g.output(os);
}

////////////////////////////////////////////////////////////////////////////////

/*
  Section 8: Domains (grids with multi-threading)

  Remember that the grid_t type above is still single-threaded. We
  introduced a bit of fancy-ness, but mostly for software engineering
  purposes (separating cells and grids, which are essentially a
  vector<cell_t>), and to demonstrate a functional programming style.

  Now we take this one step further and add multi-threading into the
  mix. To keep things simple, we restrict ourselves to C++14 features.
  Very unfortunately, this won't be efficient unless you run this on
  OS X, since all other C++14 implementations I know are based on GCC,
  and GCC's multi-threading employs pthreads. However, replacing the
  async calls below with calls to hpx::async or qthread::async is
  straightforward in principle (except that compiling the code becomes
  a nightmare). So let's stick with standard C++ features here.

  The class domain_t represents the whole simulation domain, which is
  split into many individual pieces of type grid_t. Each grid is
  handled in an asynchronous manner. We do not introduce distributed
  computing here (again to simplify things); however, replacing the
  types shared_future with hpx::client or rpc::client is basically all
  that is needed (apart from a ton of additional boilerplate for
  "actions", which are wrappers around functions that can be called
  remotely).

  A grid_t is simply a vector<cell_t>. A domain_t is a
  vector<shared_future<grid_t>>, i.e. not just a collection of grids,
  but a collection of grids that are evaluted asynchronously. A
  shared_future is essentially a shared_ptr that, when accessed,
  automatically waits until the respective object has become
  available. A future thus doesn't affect semantics, it only serves to
  automatically order calculations correctly. A shared_future also
  automatically manages memory, keeping the grid object alive exactly
  as long as it is needed, and destructing it afterwards.

  For debugging, we introduce a wait function. This function waits
  until all grids are available. This ensures that all errors that may
  occur while calculating a domain will occure before or during the
  call to wait, and not at a random later time. In a production run,
  one would never call this wait function.

  The output function is again straightforward. To access a pointer p,
  one writes *p; to access a future f, one writes f.get(). This
  obviously needs to wait until the future is ready, but this happens
  automatically. This output function may thus run for a long time,
  being suspended many times while grids are being calculated. Of
  course, with a proper multi-threading system, suspending and
  continuing is highly efficient, each about as expensive as a
  function call.

  This also means that we need to be careful when calling the output
  function, since other routines may try to write to the output stream
  at the same time, which isn't legal. We will introduce a mechanism
  for this later on -- and no, we won't use a mutex.

  Remember how the axpy routine in grid_t simply wraps the axpy
  routine in cell_t, looping over each cell. Here in domain_t, we wrap
  the axpy routine in grid_t, looping over each grid as well as adding
  asynchronicity. (In a manner of speaking, grid_t added one layer of
  complexity to cell_t, while domain_t adds two layers of complexity
  to grid_t.)

  First, the axpy routine loops over each grid, and introduces local
  variables xi and yi for the respective grids. xi and yi have the
  types shared_future<grid_t>.

  Second, the axpy routine calls async to handle this grid
  asynchronously. In a lambda expression, we first access the futures
  xi and yi to obtain the actual grids, implicitly waiting until the
  grids are ready. Then we call the axpy routine in grid_t. However,
  since this lambda expression is inside a call to async, it will be
  evaluated at msome point in the future -- and without waiting for
  this to happen, the async call returns a future to the result (of
  grid_t::axpy), which we store. Our axpy routine here thus sets up
  the calculation for all grids, but does not wait for any calculation
  to actually finish.

  In the norm function here, we choose to return the norm directly,
  and not a future of the norm. This means that the norm routine may
  block for a long time, different from the axpy routine above. This
  is a design choice. The main argument is that it is easy to wrap a
  call to norm in async to mmake it asynchronous, while in most cases,
  one actually wants the norm and not a future of the norm. There is
  also a more abstract reason for this choice (from category theory)
  that I will not present here. After making this choice, the
  implementation of the norm function is straightforward.

  The initial and error functions are straightforward and follow the
  same pattern as the axpy function.

  The RHS function is, again, the most complex in this class.
  Different from the RHS routines for a cell or a grid, this RHS
  routine does not take boundary values as input. Instead, we define
  the boundary conditions in this function. (This is a design choice;
  it would also be possible to define the boundary conditions
  elsewhere, and pass them in as arguments.)

  The RHS function first evaluates the boundary conditions, defining
  boundary (ghost) cells just outside the domain via the analytic
  solution we employ. The boundary conditions are stored into a future
  for consistency with how the boundary cell of grids are obtained.

  The RHS routine proper consists of two parts, one for looping over
  all grids, the other for handling asynchronicity.

  First, there is a for loop over all grids. Inside this loop, we
  define local variables for the future of current grid. We also
  define futures for the boundary values. These require a case
  distinction, since the outer boundaries need to be handled different
  from inter-grid boundaries. We obtain boundary values from
  neighbouring grids by calling their get_boundary routine. (The
  boolean arguments "false" and "true" select the lower and upper
  boundary cells, respectively.) Since we only have futures to the
  neighbouring grids, and since we don't want to wait for these, we
  obtain the inter-grid boundaries asynchronously.

  Then asynchronously call the grid's RHS routine. Inside a call to
  async we define a lambda expression. This lambda first accesses the
  futures of the grid and the boundaries (and may be waiting while
  doing so).
*/

// The domain is distributed over multiple threads

struct domain_t {
  double t;

  static auto ngrids() -> ptrdiff_t {
    return div_ceil(defs->ncells, defs->ncells_per_grid);
  }
  static auto cell2grid(ptrdiff_t icell) -> ptrdiff_t {
    assert(icell >= 0 && icell < defs->ncells);
    return div_floor(icell, defs->ncells_per_grid);
  }
  // Note: shared_future would become client when running distributed.
  vector<shared_future<grid_t> > grids;

  // Wait until all grids are ready
  auto wait() const {
    for (size_t i = 0; i < grids.size(); ++i)
      grids[i].wait();
  }

  // Output
  auto output(ostream &os) const -> ostream & {
    os << "domain: t=" << t << "\n";
    for (size_t i = 0; i < grids.size(); ++i)
      os << grids[i].get();
    return os;
  }

  // Linear combination
  struct axpy {};
  domain_t(axpy, double a, const domain_t &x, const domain_t &y)
      : t(a * x.t + y.t), grids(x.grids.size()) {
    assert(y.grids.size() == grids.size());
    for (size_t i = 0; i < grids.size(); ++i) {
      const auto &xi = x.grids[i];
      const auto &yi = y.grids[i];
      grids[i] = async([a, xi, yi]() {
        return grid_t(grid_t::axpy(), a, xi.get(), yi.get());
      });
    }
  }

  // Norm
  auto norm() const {
    norm_t n;
    for (size_t i = 0; i < grids.size(); ++i)
      n = n + grids[i].get().norm();
    return n;
  }

  // Initial condition
  // Note: This would also chooses the domain decomposition when
  // running distributed.
  struct initial {};
  domain_t(initial, double t)
      : t(t), grids(div_ceil(defs->ncells, defs->ncells_per_grid)) {
    for (size_t i = 0; i < grids.size(); ++i) {
      auto imin = i * defs->ncells_per_grid;
      grids[i] =
          async([t, imin]() { return grid_t(grid_t::initial(), t, imin); });
    }
  }

  // Error
  struct error {};
  domain_t(error, const domain_t &d) : t(d.t), grids(d.grids.size()) {
    for (size_t i = 0; i < grids.size(); ++i) {
      const auto &di = d.grids[i];
      grids[i] = async([ di, t = t ]() {
        return grid_t(grid_t::error(), di.get(), t);
      });
    }
  }

  // RHS
  struct rhs {};
  domain_t(rhs, const domain_t &d)
      : t(1.0), // dt/dt
        grids(d.grids.size()) {
    size_t s = grids.size();
    // Define domain boundaries
    auto dbm = make_ready_future(cell_t(cell_t::boundary(), d.t,
                                        defs->xmin - 0.5 * defs->dx)).share();
    auto dbp = make_ready_future(cell_t(cell_t::boundary(), d.t,
                                        defs->xmax + 0.5 * defs->dx)).share();
    // Loop over grids
    for (size_t i = 0; i < s; ++i) {
      const auto &di = d.grids[i];
      // lower boundary cell
      shared_future<cell_t> bm;
      if (i == 0) {
        bm = dbm;
      } else {
        const auto &dim = d.grids[i - 1];
        bm = async([dim]() { return dim.get().get_boundary(true); });
      }
      // upper boundary cell
      shared_future<cell_t> bp;
      if (i == s - 1) {
        bp = dbp;
      } else {
        const auto &dip = d.grids[i + 1];
        bp = async([dip]() { return dip.get().get_boundary(false); });
      }
      // evaluate RHS on grid
      grids[i] = async([di, bm, bp]() {
        return grid_t(grid_t::rhs(), di.get(), bm.get(), bp.get());
      });
    }
  }
};

// Output
auto operator<<(ostream &os, const domain_t &d) -> ostream & {
  return d.output(os);
}

////////////////////////////////////////////////////////////////////////////////

/*
  Section 9: Time integration

  We have now an efficient, parallel data structure that holds the
  state for the complete domain. We can set up initial conditions,
  evaluate the RHS, calculate errors, and output the result. It is
  time to put the pieces together!

  For example, we can now write a function to integrate in time, using
  e.g. a second order Runge-Kutta integrator. This function would take
  the current state as input, evaluate the RHS, produce an
  intermediate state, evaluate the RHS again, and return the final,
  new state.

  Before we do this, let us consider a particular problem with this
  approach. Consider the case where we also want to output the RHS.
  How can we prevent the RHS from being calculated twice, wasting
  work? One approach would be to calculate the RHS as soon as we have
  a new state vector, and passing this RHS to all functions that may
  want to access it.

  On the other hand, there may be cases where we have a state vector
  and never want to evaluate the RHS. In this case, evaluating the RHS
  immediately is a waste of effort as well.

  The solution we use here is "deferred asynchronicity". async can
  produce a future that is not connected to a thread that is currently
  running, but to a thread that only starts running when the future is
  actually accessed. If the future is never accessed, the thread will
  never run. This behaviour is selected with the "launch::deferred"
  option to async.

  We thus define a data structure memoized_t that holds deferred
  futures for all (important) quantities that we may want to
  calculate. In our case, these are the RHS, the error, and the norm
  of the error. The constructor of memoized_t takes the current
  iteration n and a future to the current state as arguments. From
  this it defines the other (deferred) futures.

  (You will note lambdas with strange-looking capture statements such
  as "[state = state]". "state" is not a local variable, it is a field
  of the class, and thus it cannot be captured. What could be captured
  is "this", the pointer to the object. Of course, capturing "this"
  would be dangerous, since it would not ensure that the object does
  not get destructed. The C++14 capture expression "state = state"
  defines a new local variable "state", initialised it to "state"
  (i.e. "this->state"), and captures it. This is safe. In C++11, one
  would instead manually define a new local variable "auto state =
  state;" before the lambda, and then capture it.)

  The Runge-Kutta integrator is then a straightforward implementation
  of the algorithm. Starting from the current state s0 and its RHS r0,
  which are already defined in memoized_t, we calculate the
  intermediate state s1 by calling the domain's axpy function. (The
  C++ syntax for async and lambdas spreads this call over multiple
  lines because various boilerplate items have to be added.)
  Similarly, the RHS r1 of the intermediate state is calculated by
  calling the domain's rhs function. Finally we calculate the final
  state, which we return.

  We notice a pattern here that we have already observed earlier: We
  have a future of msome object (e.g. the state s0), and want to
  asynchronously call a function that calculates it (e.g. domain_t's
  rhs). To do so, we need to call async, define a lambda, capture the
  futures; finally inside the lambda, we obtain the values of the
  futures and call the actual function.

  Similar to containers such as list or vector, shared_ptr and
  shared_future are _functors_ in the sense of category theory. In a
  practical sense, shared_ptr and shared_function are containers that
  only hold one element. C++ disagrees -- shared_ptr and shared_future
  do not satisfy the C++ definition of a container -- but in a
  practical sense, this is true. The same "map" mechanism that we
  described above for traversing vectors and trees could be applied to
  futures as well to simplify this code. For example, the line

  auto r1 =
      async([s1]() { return domain_t(domain_t::rhs(), s1.get()); }).share();

  could be written as

  auto r1 = s1.map(domain_t::rhs);

  where a fictitious member function "map" of shared_future would
  automatically call "get" and "async". Alas, this is not to be in
  this version of the C++ standard -- maybe in a few years. (A C++
  function "then" is in planning that handles calling "async", and HPX
  offers a function "unwrapped" that handles calling "get".)

  We also need to handle output. As mentioned before, one cannot
  simply output to an ostream in a multi-threaded environment, as one
  must mmake sure that only one thread at a time accesses a stream.

  We define two functions do_info_output (for screen output) and
  do_file_output (for file output) that perform the actual output.
  They obtain values from futures as necessary, and then produce
  simple ASCII output, which is sufficient for this tutorial. These
  functions only perform the actual output; they neither prevent
  conflicts nor try to ensure that output happens in the correct
  order.

  The functions info_output and similarly file_output do this. They
  are called once per iteration; they decide whether output should
  happen at this iteration, and if so, call the output routine
  asynchronously. They also ensure that output happens in the correct
  order by using "tokens" that are passed around in futures. Here we
  use the respective stream as token.

  A routine that wants to schedule output needs to have an input
  argument that is a future to the output stream. It schedules the
  actual output asynchronously (via async), which returns a new future
  (to the output stream). This new token is then returned to the
  caller. This allows daisy-chaining output, ensuring that the output
  appears in the correct order and without conflicts, although it is
  scheduled asynchronously.

  To mmake this work, it is necessary that the output streams are never
  handled directly (except in the actual output routines), but are
  always wrapped in futures and are always used in the manner
  described above. The initial futures are created at program startup,
  or when a file is opened. For technical reasons, I prefer putting
  ostream* into futures instead of ostream&.
*/

// Memoized data for the current iteration

struct memoized_t {
  ptrdiff_t n;
  shared_future<domain_t> state;
  shared_future<domain_t> rhs;
  shared_future<domain_t> error;
  shared_future<norm_t> error_norm;
  memoized_t(ptrdiff_t n, const shared_future<domain_t> &state)
      : n(n), state(state) {
    // We construct the fields deferred to avoid unnecessary work
    rhs = async(launch::deferred, [state = state]() {
      return domain_t(domain_t::rhs(), state.get());
    });
    error = async(launch::deferred, [state = state]() {
      return domain_t(domain_t::error(), state.get());
    });
    error_norm = async(launch::deferred,
                       [error = error]() { return error.get().norm(); });
  }
};

// RK2
auto rk2(const shared_ptr<memoized_t> &m) -> shared_future<domain_t> {
  const auto &s0 = m->state;
  const auto &r0 = m->rhs;
  // Step 1
  auto s1 = async([s0, r0]() {
                    return domain_t(domain_t::axpy(), 0.5 * defs->dt, r0.get(),
                                    s0.get());
                  }).share();
  auto r1 =
      async([s1]() { return domain_t(domain_t::rhs(), s1.get()); }).share();
  // Step 2
  return async([s0, r1]() {
    return domain_t(domain_t::axpy(), defs->dt, r1.get(), s0.get());
  });
}

// Output
auto do_info_output(ostream &os, const shared_ptr<memoized_t> &m) -> ostream & {
  const domain_t &s = m->state.get();
  const norm_t &en = m->error_norm.get();
  auto cell_size = cell_t().norm().count;
  auto ncells = en.count / cell_size;
  os << "n=" << m->n << " t=" << s.t << " "
     << "ncells: " << ncells << " "
     << "L2-norm[error]: " << en.norm2() << "\n" << flush;
  return os;
}

// Note: fos is passed around as a token, ensuring that output occurs
// in order and without conflicts
auto info_output(const shared_future<ostream *> &fos,
                 const shared_ptr<memoized_t> &m) -> shared_future<ostream *> {
  if (!do_this_time(m->n, defs->info_every))
    return fos;
  return async([fos, m]() { return &do_info_output(*fos.get(), m); });
}

auto do_file_output(ostream &os, const shared_ptr<memoized_t> &m) -> ostream & {
  const domain_t &s = m->state.get();
  const domain_t &r = m->rhs.get();
  const norm_t &en = m->error_norm.get();
  auto cell_size = cell_t().norm().count;
  auto ncells = en.count / cell_size;
  os << "State: " << s << "RHS: " << r << "ncells: " << ncells << "\n"
     << "L2-norm[error]: " << en.norm2() << "\n"
     << "\n" << flush;
  return os;
}

auto file_output(shared_future<ostream *> fos, const shared_ptr<memoized_t> &m)
    -> shared_future<ostream *> {
  if (!do_this_time(m->n, defs->file_every))
    return fos;
  return async([fos, m]() { return &do_file_output(*fos.get(), m); });
}

////////////////////////////////////////////////////////////////////////////////

/*
  Section 10: Main program

  For the main program, we use a different programming style. Instead
  of a functional style (which would easily be possible), we structure
  it like a scripting language, i.e. as could be written in Python.
  This is fine since (a) using a scripting langage as high-level
  driver has a certain appeal, and (b) the high-level driver has a
  very simple structure, so that an imperative design does not mmake
  things difficult to understand.

  We begin by defining a helper class stats_t to keep track of
  run-time statistics. Here we only measure time; in a more complex
  setting, we would also count things such as threads, messages,
  transferred data, etc.

  Our main program consists of three parts: Setup, initialization, and
  evolution.

  Setup is the phase before any physics happens; here we set up the
  run-time parameters, open the output file, and create the futures
  wrapping the output streams that we use for token passing.

  In the initialization phase we create the grids and set up the
  initial conditions, and output them. We first create the initial
  state s, then the memoization object m, then we call the output
  routines that may (or may not) output data. For debugging /
  benchmarking, we also include a rate limiter that may (depending on
  parameter settings) wait until all calculations have finished. In a
  production run, this rate limiter would of course be inactive, so
  that the time evolution can begin while the output routines are
  still busy.

  The third and main phase is evolution. It consists of a while loop
  that checks the termination condition, and performs a time step and
  output at each iteration. A new state vector is calculated via the
  rk2 function, and a new memoization object m is calculated from it.
  Output and rate limiting (if desired) occur as during
  initialization.

  It is important to ensure that checking the termination condition
  does not implicitly serialize the execution. For example, aborting
  when the error norm is too large would do just this: To examine this
  condition, one needs the error norm, which requires the error, which
  requires the state vector; thus, all evolution steps would be
  performed serially. Here, we check only the iteration number n and
  the simulation time t. Due to a design choice (bug?) in this code,
  the simulation time is not immediately available, requiring a call
  to "get" to obtain the domain_t object for the current state. (Note
  that the grid_t objects are not required!)

  In the main loop, the local variables s and m are overwritten. Since
  s and m are a shared_future and shared_ptr, respectively, these
  objects are then automatically freed, if they are not needed by
  other objects or threads. In other words, this will automatically
  deallocate old, unused state vectors, as soon as they are not needed
  any more.

  After the main loop we wait for all I/O to finish, and then exit.
  Depending on how threads are scheduled, msome threads may still be
  running at this time. However, since we performed all output that we
  want to perform, these threads would not be calculating anything
  that is observable from the outside, and can thus be safely aborted.
*/

// Driver

struct stats_t {
  struct snapshot_t {
    decltype(std::chrono::high_resolution_clock::now()) time;
  };
  snapshot_t start_, stop_;
  static auto snapshot(snapshot_t &dest) {
    dest.time = std::chrono::high_resolution_clock::now();
  }
  auto start() { snapshot(start_); }
  auto stop() { snapshot(stop_); }
  stats_t() { start(); }
  auto output(ostream &os) const -> ostream & {
    auto time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            stop_.time - start_.time).count() /
                        1.0e+9;
    return os << "   Time: " << time_elapsed << " sec\n";
  }
};
auto operator<<(ostream &os, const stats_t &stats) -> ostream & {
  return stats.output(os);
}

auto main(int argc, char **argv) -> int {
  // Setup
  stats_t sstats;

  // Define run-time parameters
  defs = make_shared<defs_t>(thread::hardware_concurrency());

  shared_future<ostream *> fio = make_ready_future<ostream *>(&cout);
  ostringstream filename;
  filename << "wave.txt";
  ofstream file(filename.str(), ios_base::trunc);
  shared_future<ostream *> ffo = make_ready_future<ostream *>(&file);

  sstats.stop();
  cout << "Setup:\n" << sstats;

  // Initialization
  stats_t istats;

  shared_future<domain_t> s =
      async([]() { return domain_t(domain_t::initial(), defs->tmin); });
  shared_ptr<memoized_t> m = make_shared<memoized_t>(0, s);
  fio = info_output(fio, m);
  ffo = file_output(ffo, m);

  if (do_this_time(m->n, defs->wait_every)) {
    // Rate limiter
    s.wait();
    fio.wait();
    ffo.wait();
  }

  istats.stop();
  cout << "Initialization:\n" << istats;

  // Evolution
  stats_t estats;

  // Note: "m->state.get().t" waits until the skeleton for the state
  // is ready -- is this a good idea?
  while ((defs->nsteps < 0 || m->n < defs->nsteps) &&
         (defs->tmax < 0.0 || m->state.get().t < defs->tmax + 0.5 * defs->dt)) {

    s = rk2(m);
    m = make_shared<memoized_t>(m->n + 1, s);
    fio = info_output(fio, m);
    ffo = file_output(ffo, m);

    if (do_this_time(m->n, defs->wait_every)) {
      // Rate limiter
      s.wait();
      fio.wait();
      ffo.wait();
    }
  }

  fio.wait();
  ffo.wait();

  estats.stop();
  cout << "Evolution:\n" << estats;

  file.close();

  return 0;
}

/*
  Section 11: Other remarks

  I compile and run this code on Shelob (LSU) with the commands

  source ~/SIMFACTORY/all-all/env.sh
  $CXX $CXXFLAGS $CXX14FLAGS -Ofast -c wave-light.cc
  $CXX $CXXFLAGS $CXX14FLAGS -Ofast $LDFLAGS -o wave-light wave-light.o $LIBS
  ./wave-light

  This uses clang 3.5.0.

  This code uses msome C++14 features. In particular, not all functions
  have their return type declared explicitly, and msome lambda
  expressions use explicit initializations in capture expressions.
  Both could be converted to C++11 in a straightforward way.

  I use the tool clang-format to format the source code. This ensures
  that the code is always readable and follows a consistent and
  well-thought-out style.
*/
