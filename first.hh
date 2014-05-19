#ifndef FIRST_HH
#define FIRST_HH

// Note: HPX requires that this header is the first header to be
// included, so that an HPX header is included before any boost
// headers.
#if defined RPC_HPX
#  include "hpx.hh"
#endif

#define FIRST_HH_DONE
#else
#  ifndef FIRST_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // FIRST_HH
