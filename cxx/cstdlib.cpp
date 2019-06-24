#include "cstdlib.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace cxx {

long envtol(const char *var, const char *defaultvalue) {
  assert(var);
  const char *str = std::getenv(var);
  if (!str)
    str = defaultvalue;
  if (!str) {
    std::cerr << "Could not getenv(\"" << var << "\")\n";
    std::exit(EXIT_FAILURE);
  }
  char *str_end;
  auto res = std::strtol(str, &str_end, 10);
  if (*str_end != '\0') {
    std::cerr << "Could not strol(getenv(\"" << var << "\")=\"" << str
              << "\")\n";
    std::exit(EXIT_FAILURE);
  }
  return res;
}
} // namespace cxx
