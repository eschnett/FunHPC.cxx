# source $HOME/SIMFACTORY/all-all/env.sh

DEBUG =

CC  := ${CC}	# ${MPICC}
CXX := ${CXX}   # ${MPICXX}
FC  := ${FC}    # ${MPIFC}

# -DRPC_DISABLE_CALL_SHORTCUT
# -DRPC_HPX -DRPC_QTHREADS -DRPC_STL
CPPFLAGS :=					\
	-DHPX_LIMIT=10				\
	-DRPC_QTHREADS				\
	-DBLAS
CFLAGS   := -g -Wall ${CFLAGS}   ${MPI_CFLAGS}   ${C11FLAGS}   -march=native -fmacro-backtrace-limit=0
CXXFLAGS := -g -Wall ${CXXFLAGS} ${MPI_CXXFLAGS} ${CXX14FLAGS} -march=native -fmacro-backtrace-limit=0 -ftemplate-backtrace-limit=0
FFLAGS   := -g -Wall ${FFLAGS}   ${MPI_FFLAGS}                 -march=native

ifneq (${strip ${DEBUG}},)
  # CFLAGS   += -fsanitize=local-bounds -fstack-protector-all -ftrapv
  # CXXFLAGS += -fsanitize=local-bounds -fstack-protector-all -ftrapv
  # Enable runtime instrumentation for bug detection: address (memory errors) | thread (race detection) | undefined (miscellaneous undefined behavior)
  CFLAGS   += -fstack-protector-all -ftrapv
  CXXFLAGS += -fstack-protector-all -ftrapv
  FFLAGS   += -fcheck=bounds,do,mem,pointer,recursion -finit-character=65 -finit-integer=42424242 -finit-real=nan -fstack-protector-all -ftrapv
else
  CFLAGS   += -Ofast
  CXXFLAGS += -Ofast
  FFLAGS   += -Ofast
endif

LDFLAGS  := ${LDFLAGS} ${MPI_LDFLAGS}
LIBS     := ${MPI_LIBS} ${LIBS}



# hpx_test
EXES = bench boost_bw_lat cereal_bw_lat demo hpx_bw_lat hpx_wave hwloc_test la_demo matbench mattest mpi_bw_lat ostreaming qthread_test rpc_bw_lat tree wave wave-light

QTHREAD_SRCS = qthread_thread.cc
HPX_SRCS     = hpx.cc
HWLOC_SRCS   = hwloc.cc
LA_SRCS      = la_blocked.cc la_dense.cc
RPC_SRCS     = rpc_action.cc rpc_broadcast.cc rpc_client.cc rpc_defs.cc	\
	rpc_global_shared_ptr.cc rpc_hwloc.cc rpc_main.cc rpc_server.cc	\
	rpc_server_mpi.cc						\
	${QTHREAD_SRCS} ${HPX_SRCS} ${HWLOC_SRCS}
MATRIX_SRCS  = algorithms.cc block_matrix.cc matrix.cc

BENCH_SRCS         = bench.cc ${RPC_SRCS}
BOOST_BW_LAT_SRCS  = boost_bw_lat.cc
CEREAL_BW_LAT_SRCS = cereal_bw_lat.cc
DEMO_SRCS          = demo.cc ${RPC_SRCS}
HPX_BW_LAT_SRCS    = hpx_bw_lat.cc
HPX_TEST_SRCS      = hpx_test.cc
HPX_WAVE_SRCS      = hpx_wave.cc
HWLOC_TEST_SRCS	   = hwloc_main.cc ${HWLOC_SRCS} ${RPC_SRCS}
LA_DEMO_SRCS       = la_demo.cc ${LA_SRCS} ${RPC_SRCS}
MATBENCH_SRCS      = matbench.cc ${HWLOC_SRCS} ${MATRIX_SRCS} ${RPC_SRCS}
MATTEST_SRCS       = mattest.cc ${RPC_SRCS} ${MATRIX_SRCS}
MPI_BW_LAT_SRCS    = mpi_bw_lat.cc
OSTREAMING_SRCS    = ostreaming.cc ${RPC_SRCS}
QTHREAD_TEST_SRCS  = qthread_test.cc ${QTHREAD_SRCS}
RPC_BW_LAT_SRCS    = rpc_bw_lat.cc ${RPC_SRCS}
TREE_SRCS          = tree.cc ${RPC_SRCS}
WAVE_SRCS          = wave.cc ${RPC_SRCS}
WAVE_LIGHT_SRCS    = wave-light.cc

BENCH_OBJS         = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${BENCH_SRCS}}}}}
BOOST_BW_LAT_OBJS  = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${BOOST_BW_LAT_SRCS}}}}}
CEREAL_BW_LAT_OBJS = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${CEREAL_BW_LAT_SRCS}}}}}
RPC_BW_LAT_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${RPC_BW_LAT_SRCS}}}}}
DEMO_OBJS          = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${DEMO_SRCS}}}}}
HPX_BW_LAT_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HPX_BW_LAT_SRCS}}}}}
HPX_TEST_OBJS      = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HPX_TEST_SRCS}}}}}
HPX_WAVE_OBJS      = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HPX_WAVE_SRCS}}}}}
HWLOC_TEST_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${HWLOC_TEST_SRCS}}}}}
LA_DEMO_OBJS       = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${LA_DEMO_SRCS}}}}}
MATBENCH_OBJS      = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MATBENCH_SRCS}}}}}
MATTEST_OBJS       = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MATTEST_SRCS}}}}}
MPI_BW_LAT_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${MPI_BW_LAT_SRCS}}}}}
OSTREAMING_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${OSTREAMING_SRCS}}}}}
QTHREAD_TEST_OBJS  = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${QTHREAD_TEST_SRCS}}}}}
TREE_OBJS          = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${TREE_SRCS}}}}}
WAVE_OBJS          = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${WAVE_SRCS}}}}}
WAVE_LIGHT_OBJS    = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${patsubst %.f, %.o,  ${patsubst %.f90, %.o, ${WAVE_LIGHT_SRCS}}}}}

BENCH_DEPS         = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${BENCH_SRCS}}}}}
BOOST_BW_LAT_DEPS  = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${BOOST_BW_LAT_SRCS}}}}}
CEREAL_BW_LAT_DEPS = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${CEREAL_BW_LAT_SRCS}}}}}
DEMO_DEPS          = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${DEMO_SRCS}}}}}
HPX_BW_LAT_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HPX_BW_LAT_SRCS}}}}}
HPX_TEST_DEPS      = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HPX_TEST_SRCS}}}}}
HPX_WAVE_DEPS      = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HPX_WAVE_SRCS}}}}}
HWLOC_TEST_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${HWLOC_TEST_SRCS}}}}}
LA_DEMO_DEPS       = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${LA_DEMO_SRCS}}}}}
MATBENCH_DEPS      = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MATBENCH_SRCS}}}}}
MATTEST_DEPS       = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MATTEST_SRCS}}}}}
MPI_BW_LAT_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${MPI_BW_LAT_SRCS}}}}}
OSTREAMING_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${OSTREAMING_SRCS}}}}}
QTHREAD_TEST_DEPS  = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${QTHREAD_TEST_SRCS}}}}}
RPC_BW_LAT_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${RPC_BW_LAT_SRCS}}}}}
TREE_DEPS          = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${TREE_SRCS}}}}}
WAVE_DEPS          = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${WAVE_SRCS}}}}}
WAVE_LIGHT_DEPS    = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${patsubst %.f, , ${patsubst %.f90, , ${WAVE_LIGHT_SRCS}}}}}

OBJS = ${BENCH_OBJS} ${BOOST_BW_LAT_OBJS} ${CEREAL_BW_LAT_OBJS} ${DEMO_OBJS} ${HPX_BW_LAT_OBJS} ${HPX_TEST_OBJS} ${HPX_WAVE_OBJS} ${HWLOC_TEST_OBJS} ${LA_DEMO_OBJS} ${MATBENCH_OBJS} ${MATTEST_OBJS} ${MPI_BW_LAT_OBJS} ${OSTREAMING_OBJS} ${QTHREAD_TEST_OBJS} ${RPC_BW_LAT_OBJS} ${TREE_OBJS} ${WAVE_OBJS} ${WAVE_LIGHT_OBJS}
DEPS = ${BENCH_DEPS} ${BOOST_BW_LAT_DEPS} ${CEREAL_BW_LAT_DEPS} ${DEMO_DEPS} ${HPX_BW_LAT_DEPS} ${HPX_TEST_DEPS} ${HPX_WAVE_DEPS} ${HWLOC_TEST_DEPS} ${LA_DEMO_DEPS} ${MATBENCH_DEPS} ${MATTEST_DEPS} ${MPI_BW_LAT_DEPS} ${OSTREAMING_DEPS} ${QTHREAD_TEST_DEPS} ${RPC_BW_LAT_DEPS} ${TREE_DEPS} ${WAVE_DEPS} ${WAVE_LIGHT_DEPS}



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
boost_bw_lat: ${BOOST_BW_LAT_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
cereal_bw_lat: ${CEREAL_BW_LAT_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
demo: ${DEMO_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
hpx_bw_lat: ${HPX_BW_LAT_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
hpx_test: ${HPX_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
hpx_wave: ${HPX_WAVE_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
hwloc_test: ${HWLOC_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
la_demo: ${LA_DEMO_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
matbench: ${MATBENCH_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
mattest: ${MATTEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
mpi_bw_lat: ${MPI_BW_LAT_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
ostreaming: ${OSTREAMING_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
qthread_test: ${QTHREAD_TEST_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
rpc_bw_lat: ${RPC_BW_LAT_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
tree: ${TREE_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
wave: ${WAVE_OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
wave-light: ${WAVE_LIGHT_OBJS}

%.h.pch: %.h
	${CC} ${CPPFLAGS} ${CFLAGS} -x c-header -o $@ $*.h

%.hh.pch: %.hh
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -x c++-header -o $@ $*.hh

%.o: %.c
	${CC} -MD ${CPPFLAGS} ${CFLAGS} -o $@.tmp -c $*.c
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.o: %.cc
	${CXX} -MD ${CPPFLAGS} ${CXXFLAGS} -o $@.tmp -c $*.cc
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.s: %.cc
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -S $*.cc

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



format: $(addprefix format-, $(wildcard *.hh *.cc))
format-%:
	@clang-format -i $*



clean:
	${RM} hpx_hpx.hh.pch rpc.hh.pch
	${RM} ${DEPS} ${OBJS} ${EXES}
	${RM} index.html
	${RM} mpi-rpc.aux mpi-rpc.log mpi-rpc.out mpi-rpc.pdf mpi-rpc.tex

.PHONY: all clean doc format

.SUFFIXES:

-include ${DEPS}
