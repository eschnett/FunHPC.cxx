CC       = openmpicc
CXX      = openmpic++
CPPFLAGS = -I/opt/local/include
CCFLAGS  = -Wall -Wno-deprecated-declarations -g -std=c99 -march=native
CXXFLAGS = -Wall -Wno-deprecated-declarations -g -std=c++11 -march=native
LDFLAGS  = -L/opt/local/lib
LIBS     = -lboost_mpi-mt

HDRS = constant_id.hh global_ptr.hh mpi_rpc.hh rpc_call.hh  rpc_defs.hh rpc_main.hh
OBJS = rpc_defs.o rpc_main.o demo.o
EXE  = demo



all: ${EXE}

${EXE}: ${OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o: %.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.c

%.o: %.cc
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $*.cc

${OBJS}: ${HDRS}

clean:
	${RM} ${OBJS} ${EXE}

.PHONY: all clean
