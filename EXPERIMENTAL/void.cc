#include <tuple>

double add(std::tuple<>, double x, double y) { return x + y; }

std::tuple<> addto(double &x, double y) {
  x += y;
  return {};
}
