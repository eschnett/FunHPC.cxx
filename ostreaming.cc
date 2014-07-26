#include "cxx_ostreaming.hh"
#include "cxx_monad_operators.hh"

#include <ostream>

cxx::ostreamer inner() { return cxx::make_ostreamer("Hello, "); }

void simple(std::ostream &os) {
  cxx::ostreamer ostr = inner();
  cxx::ostreamer world = cxx::make_ostreamer("World!");
  ostr = ostr << world << "\n";
  ostr(os);
}

cxx::ostreaming<int> f(int x) { return cxx::unit<cxx::ostreaming>(x + 1); }
int g(int x) { return x + 1; }
cxx::ostreaming<int> h(int x) {
  cxx::ostreamer output = cxx::make_ostreamer(x) << "\n";
  int result = x + 1;
  return { output, result };
}
cxx::ostreaming<int> h0() {
  cxx::ostreamer output = cxx::make_ostreamer("\n");
  int result = 10;
  return { output, result };
}

void real(std::ostream &os) {
  auto out1 = f(1);
  auto out2 = cxx::fmap(g, out1);
  auto out3 = cxx::join(fmap(h, out2));
  auto out4 = out3 >>= h;
  auto out5 = out4 >> h0();
  auto result = out5.get(os);
  os << "result was " << result << "\n";
}

void easy(std::ostream &os) {
  auto out1 = (f(1) >>= h) >> h0();
  auto result = out1.get(os);
  os << "result was " << result << "\n";
}

int rpc_main(int argc, char **argv) {
  simple(std::cout);
  real(std::cout);
  easy(std::cout);
  return 0;
}
