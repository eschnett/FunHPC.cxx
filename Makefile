CC       = env OMPI_CC=clang openmpicc
CXX      = env OMPI_CXX=clang++ openmpic++
CPPFLAGS = -I/opt/local/include -DBOOST_MPI_HOMOGENEOUS -DRPC_DISABLE_CALL_SHORTCUT
CCFLAGS  = -Wall -Wno-deprecated-declarations -g -std=c99   -march=native # -Ofast
CXXFLAGS = -Wall -Wno-deprecated-declarations -g -std=c++11 -march=native # -Ofast
LDFLAGS  = -L/opt/local/lib
LIBS     = -lboost_mpi-mt -lboost_serialization-mt

SRCS = rpc_main.cc rpc_server.cc
OBJS = ${patsubst %.c, %.o, ${patsubst %.cc, %.o, ${SRCS}}}
DEPS = ${patsubst %.c, %.d, ${patsubst %.cc, %.d, ${SRCS}}}
EXES = demo bench



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

bench: bench.o ${OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}
demo: demo.o ${OBJS}
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
	${RM} ${DEPS} ${OBJS} ${EXE}

.PHONY: all clean

-include ${DEPS}
