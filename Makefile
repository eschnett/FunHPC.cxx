# source $HOME/SIMFACTORY/all-all/env.sh

CC  := ${MPICC}
CXX := ${MPICXX}
FC  := ${MPIFC}

# -DRPC_DISABLE_CALL_SHORTCUT
# -DRPC_DEBUG_MISSING_EXPORTS
# -DRPC_HPX -DRPC_QTHREADS -DRPC_STL
CPPFLAGS := \
	-DBOOST_MPI_HOMOGENEOUS \
	-DRPC_QTHREADS \
	-DBLAS
CFLAGS   := -g ${CFLAGS}   ${C99FLAGS}   -march=native
CXXFLAGS := -g ${CXXFLAGS} ${CXX11FLAGS} -march=native -ftemplate-backtrace-limit=0
FFLAGS   := -g ${FFLAGS}                 -march=native

# # CFLAGS   += -fsanitize=local-bounds -fstack-protector-all -ftrapv
# # CXXFLAGS += -fsanitize=local-bounds -fstack-protector-all -ftrapv
# # Enable runtime instrumentation for bug detection: address (memory errors) | thread (race detection) | undefined (miscellaneous undefined behavior)
# CFLAGS   += -fstack-protector-all -ftrapv
# CXXFLAGS += -fstack-protector-all -ftrapv
# FFLAGS   += -fcheck=bounds,do,mem,pointer,recursion -finit-character=65 -finit-integer=42424242 -finit-real=nan -fstack-protector-all -ftrapv

CFLAGS   += -Ofast
CXXFLAGS += -Ofast
FFLAGS   += -Ofast

LDFLAGS  := ${MPI_LDFLAGS} ${LDFLAGS}
LIBS     := ${MPI_LIBS} ${LIBS}



# hpx_test
EXES = bench demo hwloc_test matbench mattest qthread_test

BLAS_SRCS    = # daxpy.f dcopy.f dgemm.f dgemv.f dnrm2.f dscal.f lsame.f xerbla.f
QTHREAD_SRCS = qthread_thread.cc
HPX_SRCS     = # hpx.cc
HWLOC_SRCS   = hwloc.cc
RPC_SRCS     = rpc_broadcast.cc rpc_defs.cc rpc_global_shared_ptr.cc \
	rpc_main.cc rpc_server.cc rpc_server_mpi.cc \
	${QTHREAD_SRCS} ${HPX_SRCS} ${HWLOC_SRCS}
MATRIX_SRCS  = algorithms.cc block_matrix.cc matrix.cc

BENCH_SRCS        = bench.cc ${RPC_SRCS}
DEMO_SRCS         = demo.cc ${RPC_SRCS}
HPX_TEST_SRCS     = hpx_test.cc
HWLOC_TEST_SRCS	  = hwloc_main.cc ${HWLOC_SRCS} ${RPC_SRCS}
MATBENCH_SRCS     = matbench.cc ${BLAS_SRCS} ${HWLOC_SRCS} ${MATRIX_SRCS} ${RPC_SRCS}
MATTEST_SRCS      = mattest.cc ${BLAS_SRCS} ${RPC_SRCS} ${MATRIX_SRCS}
QTHREAD_TEST_SRCS = qthread_test.cc ${QTHREAD_SRCS}

BENCH_OBJS        = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${BENCH_SRCS}}}}}
DEMO_OBJS         = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${DEMO_SRCS}}}}}
HPX_TEST_OBJS     = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HPX_TEST_SRCS}}}}}
HWLOC_TEST_OBJS   = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HWLOC_TEST_SRCS}}}}}
MATBENCH_OBJS     = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MATBENCH_SRCS}}}}}
MATTEST_OBJS      = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MATTEST_SRCS}}}}}
QTHREAD_TEST_OBJS = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${QTHREAD_TEST_SRCS}}}}}

BENCH_DEPS        = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${BENCH_SRCS}}}}}
DEMO_DEPS         = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${DEMO_SRCS}}}}}
HPX_TEST_DEPS     = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HPX_TEST_SRCS}}}}}
HWLOC_TEST_DEPS   = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HWLOC_TEST_SRCS}}}}}
MATBENCH_DEPS     = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MATBENCH_SRCS}}}}}
MATTEST_DEPS      = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MATTEST_SRCS}}}}}
QTHREAD_TEST_DEPS = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${QTHREAD_TEST_SRCS}}}}}

OBJS = ${BENCH_OBJS} ${DEMO_OBJS} ${HPX_TEST_OBJS} ${HWLOC_TEST_OBJS} ${MATBENCH_OBJS} ${MATTEST_OBJS} ${QTHREAD_TEST_OBJS}
DEPS = ${BENCH_DEPS} ${DEMO_DEPS} ${HPX_TEST_DEPS} ${HWLOC_TEST_DEPS} ${MATBENCH_DEPS} ${MATTEST_DEPS} ${QTHREAD_TEST_DEPS}



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
hpx_test: ${HPX_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
hwloc_test: ${HWLOC_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
matbench: ${MATBENCH_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
mattest: ${MATTEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
qthread_test: ${QTHREAD_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o: %.c
	${CC} -MD ${CPPFLAGS} ${CFLAGS} -o $@.tmp -c $*.c
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.o: %.cc
	${CXX} -MD ${CPPFLAGS} ${CXXFLAGS} -o $@.tmp -c $*.cc
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.o: %.f
	${FC} ${FFLAGS} -c $*.f

%.o: %.f90
	${FC} ${FFLAGS} -c $*.f90

${OBJS}: Makefile



doc: index.html mpi-rpc.pdf

index.html: index.rst
	rst2html-2.7.py index.rst > $@

mpi-rpc.pdf: index.rst
	rst2latex-2.7.py index.rst > mpi-rpc.tex
	pdflatex mpi-rpc.tex
	pdflatex mpi-rpc.tex



clean:
	${RM} ${DEPS} ${OBJS} ${EXES}
	${RM} index.html
	${RM} mpi-rpc.aux mpi-rpc.log mpi-rpc.out mpi-rpc.pdf mpi-rpc.tex

.PHONY: all clean doc

.SUFFIXES:

-include ${DEPS}