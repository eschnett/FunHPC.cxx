#include "rpc.hh"

#include "cxx_grid.hh"

using namespace cxx;
using namespace std;

template <typename T> using grid_ = grid<T, 3>;

auto fiota(const grid_region<3> &r, double h) {
  return iota<grid_>([h](index<3> i) {
                       return 1.0 * h * i[0] + 100.0 * h * i[1] +
                              100000.0 * h * i[2];
                     },
                     r);
}

auto fcopy(const grid<double, 3> &xs) {
  return grid<double, 3>(grid<double, 3>::copy(), xs);
}

auto fsum(const grid<double, 3> &xs) {
  return foldMap([](double x) { return x; },
                 [](double x, double y) { return x + y; }, 0.0, xs);
}

auto fneg(const grid<double, 3> &xs) {
  return fmap([](double x) { return -x; }, xs);
}

auto fadd(const grid<double, 3> &xs, const grid<double, 3> &ys) {
  return fmap2([](double x, double y) { return x + y; }, xs, ys);
}

auto flaplace(const grid<double, 2> &xs,
              const boundaries<grid<double, 2>, 2> &bs,
              double h) -> grid<double, 2> {
  double ih2 = 1.0 / (h * h);
  return stencil_fmap([ih2](double x, const boundaries<double, 2> &bs) {
                        double d2x = bs(0, false) - 2 * x + bs(0, true);
                        double d2y = bs(1, false) - 2 * x + bs(1, true);
                        return (d2x + d2y) * ih2;
                      },
                      [](double x, ptrdiff_t, bool) { return x; }, xs, bs);
}

int rpc_main(int argc, char **argv) { return 0; }
