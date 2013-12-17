CC       = env OMPI_CC=clang openmpicc
CXX      = env OMPI_CXX=clang++ openmpic++
F77	 = env OMPI_FC=dragonegg-3.3-gfortran-mp-4.8 openmpif77
F90	 = env OMPI_FC=dragonegg-3.3-gfortran-mp-4.8 openmpif90

CPPFLAGS = -I/opt/local/include -DBOOST_MPI_HOMOGENEOUS # -DRPC_DISABLE_CALL_SHORTCUT
CCFLAGS  = -Wall -Wno-deprecated-declarations -g -std=c99   -march=native # -Ofast
CXXFLAGS = -Wall -Wno-deprecated-declarations -g -std=c++11 -march=native # -Ofast
# -fplugin-arg-dragonegg-llvm-option=-mtriple:x86_64-apple-macosx10.9.0
F77FLAGS = -Wall -g -march=native # -Ofast -fplugin-arg-llvm33gcc48-enable-gcc-optzns
F90FLAGS = -Wall -g -march=native # -Ofast -fplugin-arg-llvm33gcc48-enable-gcc-optzns

LDFLAGS  = -L/opt/local/lib -L/opt/local/lib/gcc48
LIBS     = -lboost_mpi-mt -lboost_serialization-mt -lgfortran

EXES = bench demo matmul

RPC_SRCS    = rpc_defs.cc rpc_global_shared_ptr.cc rpc_main.cc rpc_server.cc
BENCH_SRCS  = bench.cc ${RPC_SRCS}
DEMO_SRCS   = demo.cc ${RPC_SRCS}
MATMUL_SRCS = algorithms.cc block_matrix.cc matmul.cc matrix.cc \
	daxpy.f dcopy.f dgemm.f dgemv.f dnrm2.f dscal.f lsame.f xerbla.f \
	${RPC_SRCS}

BENCH_OBJS  = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${BENCH_SRCS}}}}}
DEMO_OBJS   = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${DEMO_SRCS}}}}}
MATMUL_OBJS = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MATMUL_SRCS}}}}}

BENCH_DEPS  = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${BENCH_SRCS}}}}}
DEMO_DEPS   = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${DEMO_SRCS}}}}}
MATMUL_DEPS = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MATMUL_SRCS}}}}}

OBJS = ${BENCH_OBJS} ${DEMO_OBJS} ${MATMUL_OBJS}
DEPS = ${BENCH_DEPS} ${DEMO_DEPS} ${MATMUL_DEPS}



# Taken from <http://mad-scientist.net/make/autodep.html> as written
# by Paul D. Smith <psmith@gnu.org>, originally developed by Tom
# Tromey <tromey@cygnus.com>
PROCESS_DEPENDENCIES =					\
	sed -e 's/$@.tmp/$@/g' < $*.o.d > $*.d &&	\
	sed -e 's/\#.*//'				\
		-e 's/^[^:]*: *//'			\
		-e 's/ *\\$$//'				\
		-e '/^$$/ d'				\
		-e 's/$$/ :/' < $*.o.d >> $*.d &&	\
	rm -f $*.o.d



all: ${EXES}

bench: ${BENCH_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
demo: ${DEMO_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
matmul: ${MATMUL_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o: %.c
	${CC} -MD ${CPPFLAGS} ${CFLAGS} -o $@.tmp -c -emit-llvm $*.c
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.o: %.cc
	${CXX} -MD ${CPPFLAGS} ${CXXFLAGS} -o $@.tmp -c -emit-llvm $*.cc
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.ll: %.f
	${F77} ${F77FLAGS} -o $@ -S -fplugin-arg-llvm33gcc48-emit-ir $*.f

%.ll: %.f90
	${F90} ${F90FLAGS} -o $@ -S -fplugin-arg-llvm33gcc48-emit-ir $*.f90

%.o: %.ll
	llvm-as-mp-3.3 -o $@ $*.ll

${OBJS}: Makefile

clean:
	${RM} ${DEPS} ${OBJS} ${OBJS:.o=.ll} ${EXES}

.PHONY: all clean

.SUFFIXES:

-include ${DEPS}
