#include "rpc_defs.hh"

#include <cstdlib>
#include <cstring>
#include <ios>
#include <string>
#include <sstream>

namespace rpc {

mutex io_mutex;

char *strdup(const char *str) {
  std::size_t sz = std::strlen(str) + 1;
  char *ret = static_cast<char *>(std::malloc(sz));
  std::memcpy(ret, str, sz);
  return ret;
}

const char *make_hash_string(size_t hash_code) {
  std::ostringstream os;
  os << std::hex << hash_code << "#";
  return strdup(os.str().c_str());
}

const char *make_hash_string(const char *str) {
  return make_hash_string(std::hash<std::string>()(str));
}
}
