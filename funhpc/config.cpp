#include <funhpc/config.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace funhpc {

int version_major() { return FUNHPC_VERSION_MAJOR; }
int version_minor() { return FUNHPC_VERSION_MINOR; }
int version_patch() { return FUNHPC_VERSION_PATCH; }

const char *version() { return FUNHPC_VERSION; }

void check_version(const char *header_version) {
  if (std::strcmp(header_version, version()) != 0) {
    std::cerr
        << "Version mismatch detected -- aborting.\n"
        << "  Include headers have version " << header_version << ",\n"
        << "  Linked library has version " << version() << ".\n"
        << "(The versions of the include headers and linked libraries differ.\n"
        << "This points to an improperly installed library or\n"
        << "improperly installed application.)\n";
    std::exit(EXIT_FAILURE);
  }
}
} // namespace funhpc
