# Makefile for FunHPC

CEREAL_NAME     = cereal-1.1.0
CEREAL_URL      = https://github.com/USCiLab/cereal/archive/v1.1.0.tar.gz
CEREAL_DIR      = $(abspath ./$(CEREAL_NAME))
CEREAL_CPPFLAGS = -DCEREAL_ENABLE_RAW_POINTER_SERIALIZATION
CEREAL_INCDIRS  = $(CEREAL_DIR)/include
CEREAL_LIBDIRS  =
CEREAL_LIBS     =

GCC_NAME     = gcc-4.9.2
GCC_URL      = http://ftp.gnu.org/gnu/gcc/gcc-4.9.2/gcc-4.9.2.tar.bz2
GCC_DIR      = $(abspath ./$(GCC_NAME))
GCC_CPPFLAGS =
GCC_INCDIRS  = $(GCC_DIR)/include
GCC_LIBDIRS  =
GCC_LIBS     =

GOOGLETEST_NAME     = gtest-1.7.0
GOOGLETEST_URL      = https://googletest.googlecode.com/files/gtest-1.7.0.zip
GOOGLETEST_DIR      = $(abspath ./$(GOOGLETEST_NAME))
GOOGLETEST_CPPFLAGS =
GOOGLETEST_INCDIRS  = $(GOOGLETEST_DIR)/include $(GOOGLETEST_DIR)
GOOGLETEST_LIBDIRS  = $(GOOGLETEST_DIR)/src
GOOGLETEST_LIBS     =

HWLOC_NAME     = hwloc-1.10.1
HWLOC_URL      = http://www.open-mpi.org/software/hwloc/v1.10/downloads/hwloc-1.10.1.tar.bz2
HWLOC_DIR      = $(abspath ./$(HWLOC_NAME))
HWLOC_CPPFLAGS =
HWLOC_INCDIRS  = $(HWLOC_DIR)/include
HWLOC_LIBDIRS  = $(HWLOC_DIR)/lib
HWLOC_LIBS     = hwloc

JEMALLOC_NAME     = jemalloc-3.6.0
JEMALLOC_URL      = http://www.canonware.com/download/jemalloc/jemalloc-3.6.0.tar.bz2
JEMALLOC_DIR      = $(abspath ./$(JEMALLOC_NAME))
JEMALLOC_CPPFLAGS =
JEMALLOC_INCDIRS  = $(JEMALLOC_DIR)/include
JEMALLOC_LIBDIRS  = $(JEMALLOC_DIR)/lib
JEMALLOC_LIBS     = jemalloc pthread # Note: JEMALLOC_LIBS must come last

OPENMPI_NAME     = openmpi-1.8.4
OPENMPI_URL      = http://www.open-mpi.org/software/ompi/v1.8/downloads/openmpi-1.8.4.tar.bz2
OPENMPI_DIR      = $(abspath ./$(OPENMPI_NAME))
OPENMPI_CPPFLAGS =
OPENMPI_INCDIRS  = $(HWLOC_INCDIRS)
OPENMPI_LIBDIRS  = $(HWLOC_LIBDIRS)
OPENMPI_LIBS     = $(HWLOC_LIBS)

QTHREADS_NAME     = qthread-1.10
QTHREADS_URL      = https://qthreads.googlecode.com/files/qthread-1.10.tar.bz2
QTHREADS_DIR      = $(abspath ./$(QTHREADS_NAME))
QTHREADS_CPPFLAGS =
QTHREADS_INCDIRS  = $(QTHREADS_DIR)/include $(HWLOC_INCDIRS)
QTHREADS_LIBDIRS  = $(QTHREADS_DIR)/lib $(HWLOC_LIBDIRS)
QTHREADS_LIBS     = qthread pthread $(HWLOC_LIBS)

INCDIRS =					\
	$(abspath .)				\
	$(CEREAL_INCDIRS)			\
	$(GOOGLETEST_INCDIRS)			\
	$(HWLOC_INCDIRS)			\
	$(JEMALLOC_INCDIRS)			\
	$(OPENMPI_INCDIRS)			\
	$(QTHREADS_INCDIRS)
CPPFLAGS =					\
	$(INCDIRS:%=-I%)			\
	$(CEREAL_CPPFLAGS)			\
	$(GOOGLETEST_CPPFLAGS)			\
	$(HWLOC_CPPFLAGS)			\
	$(JEMALLOC_CPPFLAGS)			\
	$(OPENMPI_CPPFLAGS)			\
	$(QTHREADS_CPPFLAGS)
LIBDIRS =					\
	$(CEREAL_LIBDIRS)			\
	$(GOOGLETEST_LIBDIRS)			\
	$(HWLOC_LIBDIRS)			\
	$(JEMALLOC_LIBDIRS)			\
	$(OPENMPI_LIBDIRS)			\
	$(QTHREADS_LIBDIRS)
LDFLAGS =					\
	$(LIBDIRS:%=-L%)			\
	$(LIBDIRS:%=-Wl,-rpath,%)
LIBS =						\
	$(CEREAL_LIBS)				\
	$(GOOGLETEST_LIBS)			\
	$(HWLOC_LIBS)				\
	$(JEMALLOC_LIBS)			\
	$(OPENMPI_LIBS)				\
	$(QTHREADS_LIBS)			\
	$(JEMALLOC_LIBS)

COMPILER = clang

ifeq ($(COMPILER), clang)

CC           = clang
CXX          = clang++
CFLAGS       = -march=native -Wall -g -std=c11 -Dasm=__asm__
CXXFLAGS     = -march=native -Wall -g -std=c++14 -fmacro-backtrace-limit=0 -ftemplate-backtrace-limit=0 -Drestrict=__restrict__
DEBUGFLAGS   = # -D_GLIBCXX_DEBUG
OPTFLAGS     = -O3 -flto -DNDEBUG -Wno-unused-variable
CFLAGS_EXT   = -march=native -Wall -g -O3
CXXFLAGS_EXT = -march=native -Wall -g -O3

else ifeq ($(COMPILER), gcc)

CC           = gcc
CXX          = g++
CFLAGS       = -m128bit-long-double -march=native -Wall -g -std=c11 -Dasm=__asm__
CXXFLAGS     = -m128bit-long-double -march=native -Wall -g -std=c++14 -Drestrict=__restrict__
DEBUGFLAGS   = -D_GLIBCXX_DEBUG
OPTFLAGS     = # -O3 -flto -DNDEBUG
CFLAGS_EXT   = -m128bit-long-double -march=native -Wall -g -O3
CXXFLAGS_EXT = -m128bit-long-double -march=native -Wall -g -O3

endif

MPICC       = env "OMPI_CC=$(CC)" $(OPENMPI_DIR)/bin/mpicc
MPICXX      = env "OMPI_CXX=$(CXX)" $(OPENMPI_DIR)/bin/mpicxx
MPICPPFLAGS = $(CPPFLAGS)
MPICFLAGS   = $(CFLAGS)
MPICXXFLAGS = $(CXXFLAGS)
MPILDFLAGS  = $(LDFLAGS)
MPILIBS     = $(LIBS)
MPIRUN      = $(OPENMPI_DIR)/bin/mpirun

HDRS =						\
	adt/array.hpp				\
	adt/either.hpp				\
	adt/grid.hpp				\
	adt/maybe.hpp				\
	adt/nested.hpp				\
	adt/tree.hpp				\
	cxx/apply.hpp				\
	cxx/cstdlib.hpp				\
	cxx/invoke.hpp				\
	cxx/serialize.hpp			\
	cxx/task.hpp				\
	cxx/utility.hpp				\
	fun/array.hpp				\
	fun/either.hpp				\
	fun/fun.hpp				\
	fun/function.hpp			\
	fun/grid.hpp				\
	fun/maybe.hpp				\
	fun/nested.hpp				\
	fun/pair.hpp				\
	fun/proxy.hpp				\
	fun/shared_future.hpp			\
	fun/shared_ptr.hpp			\
	fun/topology.hpp			\
	fun/tree.hpp				\
	fun/vector.hpp				\
	funhpc/async.hpp			\
	funhpc/hwloc.hpp			\
	funhpc/main.hpp				\
	funhpc/proxy.hpp			\
	funhpc/rexec.hpp			\
	funhpc/rptr.hpp				\
	funhpc/serialize_shared_future.hpp	\
	funhpc/server.hpp			\
	funhpc/shared_rptr.hpp			\
	qthread/future.hpp			\
	qthread/mutex.hpp			\
	qthread/thread.hpp
SRCS =						\
	cxx/serialize.cc			\
	qthread/future.cc
FUNHPC_SRCS =					\
	funhpc/hwloc.cc				\
	funhpc/main.cc				\
	funhpc/server.cc
TEST_SRCS =					\
	adt/array_test.cc			\
	adt/either_test.cc			\
	adt/grid_test.cc			\
	adt/maybe_test.cc			\
	adt/nested_test.cc			\
	adt/tree_test.cc			\
	cxx/apply_test.cc			\
	cxx/invoke_test.cc			\
	cxx/serialize_test.cc			\
	cxx/task_test.cc			\
	cxx/utility_test.cc			\
	fun/array_test.cc			\
	fun/either_test.cc			\
	fun/fun_test.cc				\
	fun/function_test.cc			\
	fun/grid_test.cc			\
	fun/maybe_test.cc			\
	fun/nested_test.cc			\
	fun/pair_test.cc			\
	fun/shared_future_test.cc		\
	fun/shared_ptr_test.cc			\
	fun/tree_test.cc			\
	fun/vector_test.cc			\
	qthread/future_test.cc			\
	qthread/future_test_std.cc		\
	qthread/mutex_test.cc			\
	qthread/mutex_test_std.cc		\
	qthread/thread_test.cc			\
	qthread/thread_test_std.cc
FUNHPC_TEST_SRCS =				\
	fun/proxy_test.cc			\
	funhpc/async_test.cc			\
	funhpc/proxy_test.cc			\
	funhpc/rexec_test.cc			\
	funhpc/shared_rptr_test.cc		\
	funhpc/test_main.cc
FUNHPC_EXAMPLE_SRCS =				\
	examples/benchmark.cc			\
	examples/fibonacci.cc			\
	examples/hello.cc			\
	examples/pingpong.cc			\
	examples/wave1d.cc
ALL_SRCS =							\
	$(SRCS) $(FUNHPC_SRCS)					\
	$(TEST_SRCS) $(FUNHPC_TEST_SRCS) $(FUNHPC_EXAMPLE_SRCS)

# Taken from <http://mad-scientist.net/make/autodep.html> as written
# by Paul D. Smith <psmith@gnu.org>, originally developed by Tom
# Tromey <tromey@cygnus.com>
PROCESS_DEPENDENCIES =							      \
	{								      \
	perl -p -e 's{$*.o.tmp}{$*.o}g' < $*.o.d &&			      \
	perl -p -e 's{\#.*}{};s{^[^:]*: *}{};s{ *\\$$}{};s{$$}{ :}' < $*.o.d; \
	} > $*.d &&							      \
	$(RM) $*.o.d

comma := ,
empty :=
space := $(empty) $(empty)

all: objs selftest selftest-funhpc $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%)
.PHONY: all

### format ###

format: $(HDRS:%=%.fmt) $(ALL_SRCS:%=%.fmt)
.PHONY: format
%.fmt: % Makefile 
	-clang-format -style=llvm -i $* && : > $*.fmt

### compile ###

objs: $(ALL_SRCS:%.cc=%.o)
.PHONY: objs
$(ALL_SRCS:%.cc=%.o): | format cereal gtest jemalloc hwloc openmpi qthreads
%.o: %.cc Makefile
	$(MPICXX) -MD $(MPICPPFLAGS) $(MPICXXFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) \
	    -c -o $*.o.tmp $*.cc
	@$(PROCESS_DEPENDENCIES)
	@mv $*.o.tmp $*.o

### run an application ###

# These two can be overridden on the command line
NPROCS := 1
NSHEPHERDS :=						\
	$(or $(shell $(HWLOC_DIR)/bin/hwloc-info |	\
	    awk '/ NUMANode / { print $$3; }'), 1)
NTHREADS :=						\
	$(or $(shell $(HWLOC_DIR)/bin/hwloc-info |	\
	    awk '/ PU / { print $$3; }'), 1)
NSHEPHERDS_PER_PROC := \
	$(shell echo '($(NSHEPHERDS) + $(NPROCS) - 1) / $(NPROCS)' | bc)
NTHREADS_PER_SHEPHERD :=						       \
	$(shell echo '($(NTHREADS) + $(NPROCS) * $(NSHEPHERDS_PER_PROC) - 1) / \
	    ($(NPROCS) * $(NSHEPHERDS_PER_PROC))' | bc)

EXE = ./hello
run:
	unset LD_LIBRARY_PATH DYLD_LIBRARY_PATH &&			   \
	$(MPIRUN)							   \
	    -np $(NPROCS)						   \
	    -x "QTHREAD_NUM_SHEPHERDS=$(NSHEPHERDS_PER_PROC)"		   \
	    -x "QTHREAD_NUM_WORKERS_PER_SHEPHERD=$(NTHREADS_PER_SHEPHERD)" \
	    -x "QTHREAD_STACK_SIZE=65536"				   \
	    $(EXE)
.PHONY: run

### examples ###

examples: $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%)
	for example in $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%); do	\
	    $(MAKE) run EXE=$$example;					\
	done
	for example in $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%); do	\
	    $(MAKE) run NPROCS=2 EXE=$$example;				\
	done
.PHONY: examples
benchmark: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/benchmark.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    $(MPILIBS:%=-l%)
fibonacci: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/fibonacci.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    $(MPILIBS:%=-l%)
hello: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/hello.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    $(MPILIBS:%=-l%)
pingpong: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/pingpong.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    $(MPILIBS:%=-l%)
wave1d: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/wave1d.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    $(MPILIBS:%=-l%)

### check ###

check: selftest selftest-funhpc
	$(MAKE) run EXE=./selftest
	$(MAKE) run NPROCS=2 EXE=./selftest-funhpc
.PHONY: check
selftest: $(TEST_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -lgtest $(LIBS:%=-l%)
selftest-funhpc:							      \
	$(FUNHPC_TEST_SRCS:%.cc=%.o) $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o)
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
	    -lgtest $(MPILIBS:%=-l%)

### external ###

external:
	mkdir external

### Cereal ###

cereal: external/cereal.done
.PHONY: cereal
external/cereal.downloaded: | external
	(cd external &&				\
	    $(RM) $(notdir $(CEREAL_URL)) &&	\
	    wget $(CEREAL_URL)) &&		\
	: > $@
external/cereal.unpacked: external/cereal.downloaded
	$(RM) -r $(CEREAL_NAME) &&					\
	tar xzf external/$(notdir $(CEREAL_URL)) &&			\
	(cd $(CEREAL_NAME) &&						\
	    patch -p0 < $(abspath cereal-pointers.patch) &&		\
	    patch -p0 < $(abspath cereal-to_string.patch)) &&	\
	: > $@
external/cereal.done: external/cereal.unpacked
	: > $@

### GCC

gcc: external/gcc.done
.PHONY: gcc
external/gcc.downloaded: | external
	(cd external &&								  \
	    $(RM) $(notdir $(GCC_URL)) &&					  \
	    wget $(GCC_URL) &&							  \
	    $(RM) gmp-4.3.2.tar.bz2 mpfr-2.4.2.tar.bz2 mpc-0.8.1.tar.gz		  \
		isl-0.12.2.tar.bz2 cloog-0.18.1.tar.gz) &&			  \
	    wget https://gmplib.org/download/gmp/gmp-4.3.2.tar.bz2		  \
	    wget ftp://gcc.gnu.org/pub/gcc/infrastructure/mpfr-2.4.2.tar.bz2 &&	  \
	    wget http://www.multiprecision.org/mpc/download/mpc-0.8.1.tar.gz &&	  \
	    wget ftp://gcc.gnu.org/pub/gcc/infrastructure/isl-0.12.2.tar.bz2 &&	  \
	    wget ftp://gcc.gnu.org/pub/gcc/infrastructure/cloog-0.18.1.tar.gz) && \
	: > $@
external/gcc.unpacked: external/gcc.downloaded
	(cd external &&							     \
	    $(RM) -r $(GCC_NAME) &&					     \
	    tar xjf $(notdir $(GCC_URL)) &&				     \
	    $(RM) -r gmp-4.3.2 mpfr-2.4.2 mpc-0.8.1 isl-0.12.2 cloog-0.18.1 && \
	    tar xjf gmp-4.3.2.tar.bz2 &&				     \
	    tar xjf mpfr-2.4.2.tar.bz2 &&				     \
	    tar xzf mpc-0.8.1.tar.gz &&					     \
	    tar xjf isl-0.12.2.tar.bz2 &&				     \
	    tar xzf cloog-0.18.1.tar.gz &&				     \
	    cd $(GCC_NAME) &&						     \
	    ln -s ../gmp-4.3.2 gmp &&					     \
	    ln -s ../mpfr-2.4.2 mpfr &&					     \
	    ln -s ../mpc-0.8.1 mpc &&					     \
	    ln -s ../isl-0.12.2 isl &&					     \
	    ln -s ../cloog-0.18.1 cloog) &&				     \
	: > $@
external/gcc.built: external/gcc.unpacked
	+(cd external &&				\
	    $(RM) -r $(GCC_NAME)-build &&			\
	    mkdir $(GCC_NAME)-build &&			\
	    cd $(GCC_NAME)-build &&			\
	    "$(abspath external/$(GCC_NAME)/configure)"	\
		--prefix="$(GCC_DIR)"			\
		--enable-languages=c,c++,fortran	\
		--disable-multilib &&			\
	    $(MAKE)) &&					\
	: > $@
external/gcc.installed: external/gcc.built
	+(cd external &&			\
	    $(RM) -r $(GCC_DIR) &&		\
	    cd $(GCC_NAME)-build &&		\
	    $(MAKE) install) &&			\
	: > $@
external/gcc.done: external/gcc.installed
	: > $@

### Google Test ###

gtest: external/gtest.done
.PHONY: gtest
external/gtest.downloaded: | external
	(cd external &&					\
	    $(RM) $(notdir $(GOOGLETEST_URL)) &&	\
	    wget $(GOOGLETEST_URL)) &&			\
	: > $@
external/gtest.unpacked: external/gtest.downloaded
	$(RM) -r $(GOOGLETEST_NAME) &&			\
	unzip external/$(notdir $(GOOGLETEST_URL)) &&	\
	: > $@
external/gtest.built: external/gtest.unpacked
	(cd external &&							\
	    cd $(GOOGLETEST_DIR)/src &&					\
	    $(CXX) $(CPPFLAGS) $(CXXFLAGS_EXT) -c gtest-all.cc &&	\
	    $(CXX) $(CPPFLAGS) $(CXXFLAGS_EXT) -c gtest_main.cc &&	\
	    $(AR) -r -c libgtest.a gtest-all.o gtest_main.o) &&		\
	: > $@
external/gtest.done: external/gtest.built
	: > $@

### hwloc ###

hwloc: external/hwloc.done
.PHONY: hwloc
external/hwloc.downloaded: | external
	(cd external &&				\
	    $(RM) $(notdir $(HWLOC_URL)) &&	\
	    wget $(HWLOC_URL)) &&		\
	: > $@
external/hwloc.unpacked: external/hwloc.downloaded
	(cd external &&				\
	    $(RM) -r $(HWLOC_NAME) &&		\
	    tar xjf $(notdir $(HWLOC_URL))) &&	\
	: > $@
external/hwloc.built: external/hwloc.unpacked
	+(cd external &&					\
	    $(RM) -r $(HWLOC_NAME)-build &&			\
	    mkdir $(HWLOC_NAME)-build &&			\
	    cd $(HWLOC_NAME)-build &&				\
	    "$(abspath external/$(HWLOC_NAME)/configure)"	\
		--prefix="$(HWLOC_DIR)"				\
		--disable-libxml2				\
		"CC=$(CC)"					\
		"CXX=$(CXX)"					\
		"CFLAGS=$(CFLAGS_EXT)"				\
		"CXXFLAGS=$(CXXFLAGS_EXT)" &&			\
	    $(MAKE)) &&						\
	: > $@
external/hwloc.installed: external/hwloc.built
	+(cd external &&			\
	    $(RM) -r $(HWLOC_DIR) &&		\
	    cd $(HWLOC_NAME)-build &&		\
	    $(MAKE) install) &&			\
	: > $@
external/hwloc.done: external/hwloc.installed
	: > $@

### jemalloc ###

jemalloc: external/jemalloc.done
.PHONY: jemalloc
external/jemalloc.downloaded: | external
	(cd external &&				\
	    $(RM) $(notdir $(JEMALLOC_URL)) &&	\
	    wget $(JEMALLOC_URL)) &&		\
	: > $@
external/jemalloc.unpacked: external/jemalloc.downloaded
	(cd external &&					\
	    $(RM) -r $(JEMALLOC_NAME) &&			\
	    tar xjf $(notdir $(JEMALLOC_URL))) &&	\
	: > $@
external/jemalloc.built: external/jemalloc.unpacked
	+(cd external &&					\
	    $(RM) -r $(JEMALLOC_NAME)-build &&			\
	    mkdir $(JEMALLOC_NAME)-build &&			\
	    cd $(JEMALLOC_NAME)-build &&			\
	    "$(abspath external/$(JEMALLOC_NAME)/configure)"	\
		--prefix="$(JEMALLOC_DIR)"			\
		"CC=$(CC)"					\
		"CXX=$(CXX)"					\
		"CFLAGS=$(CFLAGS_EXT)"				\
		"CXXFLAGS=$(CXXFLAGS_EXT)" &&			\
	    $(MAKE)) &&						\
	: > $@
external/jemalloc.installed: external/jemalloc.built
	+(cd external &&					\
	    $(RM) -r $(JEMALLOC_DIR) &&				\
	    cd $(JEMALLOC_NAME)-build &&			\
	    $(MAKE) install) &&					\
	if command -v install_name_tool >/dev/null 2>&1; then	\
	    lib=$(JEMALLOC_DIR)/lib/libjemalloc.*.dylib &&	\
	    install_name_tool -id $$lib $$lib;			\
	fi &&							\
	: > $@
external/jemalloc.done: external/jemalloc.installed
	: > $@

### OpenMPI ###

openmpi: external/openmpi.done
.PHONY: openmpi
external/openmpi.downloaded: | external
	(cd external &&				\
	    $(RM) $(notdir $(OPENMPI_URL)) &&	\
	    wget $(OPENMPI_URL)) &&		\
	: > $@
external/openmpi.unpacked: external/openmpi.downloaded
	(cd external &&					\
	    $(RM) -r $(OPENMPI_NAME) &&			\
	    tar xjf $(notdir $(OPENMPI_URL))) &&	\
	: > $@
external/openmpi.built: external/openmpi.unpacked | hwloc
	+(cd external &&					\
	    $(RM) -r $(OPENMPI_NAME)-build &&			\
	    mkdir $(OPENMPI_NAME)-build &&			\
	    cd $(OPENMPI_NAME)-build &&				\
	    unset MPICC MPICXX &&				\
	    "$(abspath external/$(OPENMPI_NAME)/configure)"	\
		--prefix="$(OPENMPI_DIR)"			\
		--with-hwloc="$(HWLOC_DIR)"			\
		"CC=gcc"					\
		"CXX=g++"					\
		"CFLAGS=$(CFLAGS_EXT)"				\
		"CXXFLAGS=$(CXXFLAGS_EXT)" &&			\
	    $(MAKE)) &&						\
	: > $@
external/openmpi.installed: external/openmpi.built
	+(cd external &&			\
	    $(RM) -r $(OPENMPI_DIR) &&		\
	    cd $(OPENMPI_NAME)-build &&		\
	    $(MAKE) install) &&			\
	: > $@
external/openmpi.done: external/openmpi.installed
	: > $@

### Qthreads ###

qthreads: external/qthreads.done
.PHONY: qthreads
external/qthreads.downloaded: | external
	(cd external &&				\
	    $(RM) $(notdir $(QTHREADS_URL)) &&	\
	    wget $(QTHREADS_URL)) &&		\
	: > $@
external/qthreads.unpacked: external/qthreads.downloaded
	(cd external &&					\
	    $(RM) -r $(QTHREADS_NAME) &&			\
	    tar xjf $(notdir $(QTHREADS_URL)) &&	\
	    cd $(QTHREADS_NAME) &&			\
	    patch -p0 < $(abspath qthreads.patch)) &&	\
	: > $@
external/qthreads.built: external/qthreads.unpacked | hwloc
	+(cd external &&					\
	    $(RM) -r $(QTHREADS_NAME)-build &&			\
	    mkdir $(QTHREADS_NAME)-build &&			\
	    cd $(QTHREADS_NAME)-build &&			\
	    "$(abspath external/$(QTHREADS_NAME)/configure)"	\
		--prefix="$(QTHREADS_DIR)"			\
		--enable-guard-pages --enable-debug=yes		\
		--with-hwloc="$(HWLOC_DIR)"			\
		"CC=$(CC)"					\
		"CXX=$(CXX)"					\
		"CFLAGS=$(CFLAGS_EXT)"				\
		"CXXFLAGS=$(CXXFLAGS_EXT)"			\
		"LDFLAGS=-Wl,-rpath,$(HWLOC_DIR)/lib" &&	\
	    $(MAKE)) &&						\
	: > $@
external/qthreads.installed: external/qthreads.built
	+(cd external &&			\
	    $(RM) -r $(QTHREADS_DIR) &&		\
	    cd $(QTHREADS_NAME)-build &&	\
	    $(MAKE) install) &&			\
	: > $@
external/qthreads.done: external/qthreads.installed
	: > $@

### clean ###

clean:
	$(RM) $(HDRS:%=%.fmt) $(ALL_SRCS:%=%.fmt)
	$(RM) $(ALL_SRCS:%.cc=%.o) $(ALL_SRCS:%.cc=%.d)
	$(RM) selftest selftest-funhpc
	$(RM) $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%)
	$(RM) wave1d.tsv
.PHONY: clean

distclean: clean
	$(RM) -r external $(CEREAL_DIR) $(GOOGLETEST_DIR) $(HWLOC_DIR)	\
	    $(JEMALLOC_DIR) $(OPENMPI_DIR) $(QTHREADS_DIR)
.PHONY: distclean

-include $(ALL_SRCS:%.cc=%.d)
