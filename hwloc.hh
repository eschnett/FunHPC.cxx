#ifndef HWLOC_HH
#define HWLOC_HH

void hwloc_bindings(bool do_set, bool do_output);

#define HWLOC_HH_DONE
#else
#ifndef HWLOC_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // HWLOC_HH
