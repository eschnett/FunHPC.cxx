CC       = env OMPI_CC=clang openmpicc
CXX      = env OMPI_CXX=clang++ openmpic++
CPPFLAGS = -I/opt/local/include -DBOOST_MPI_HOMOGENEOUS -DRPC_DISABLE_CALL_SHORTCUT
CCFLAGS  = -Wall -Wno-deprecated-declarations -g -std=c99   -march=native # -Ofast
CXXFLAGS = -Wall -Wno-deprecated-declarations -g -std=c++11 -march=native # -Ofast
LDFLAGS  = -L/opt/local/lib
LIBS     = -lboost_mpi-mt -lboost_serialization-mt

EXES = bench demo matmul

RPC_SRCS    = rpc_defs.cc rpc_main.cc rpc_shared_global_ptr.cc rpc_server.cc
BENCH_SRCS  = bench.cc ${RPC_SRCS}
DEMO_SRCS   = demo.cc ${RPC_SRCS}
MATMUL_SRCS = algorithms.cc block_matrix.cc matmul.cc matrix.cc ${RPC_SRCS}

BENCH_OBJS  = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${BENCH_SRCS}}}
DEMO_OBJS   = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${DEMO_SRCS}}}
MATMUL_OBJS = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${MATMUL_SRCS}}}

BENCH_DEPS  = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${BENCH_SRCS}}}
DEMO_DEPS   = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${DEMO_SRCS}}}
MATMUL_DEPS = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${MATMUL_SRCS}}}

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
	${CC} -MD ${CPPFLAGS} ${CFLAGS} -o $@.tmp -c $*.c
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

%.o: %.cc
	${CXX} -MD ${CPPFLAGS} ${CXXFLAGS} -o $@.tmp -c $*.cc
	@${PROCESS_DEPENDENCIES}
	@mv $@.tmp $@

${OBJS}: Makefile

clean:
	${RM} ${DEPS} ${OBJS} ${EXES}

.PHONY: all clean

-include ${DEPS}
