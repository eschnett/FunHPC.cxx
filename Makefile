# Makefile for FunHPC

# pushd ~/software && source cereal-1.1.1/env.sh && source hwloc-1.10.1/env.sh && source jemalloc-3.6.0/env.sh && source llvm-3.6.1/env.sh && source openmpi-1.8.5/env.sh && source qthreads-1.10/env.sh && popd && make

GOOGLETEST_NAME     = gtest-1.7.0
GOOGLETEST_URL      = https://googletest.googlecode.com/files/gtest-1.7.0.zip
GOOGLETEST_DIR      = $(abspath ./$(GOOGLETEST_NAME))
GOOGLETEST_INCDIRS  = $(GOOGLETEST_DIR)/include $(GOOGLETEST_DIR)
GOOGLETEST_LIBDIRS  = $(GOOGLETEST_DIR)/src
GOOGLETEST_OBJS     = $(GOOGLETEST_DIR)/src/gtest-all.o
GOOGLETEST_MAIN_OBJ = $(GOOGLETEST_DIR)/src/gtest_main.o

INCDIRS = . $(GOOGLETEST_INCDIRS)
LIBDIRS = $(GOOGLETEST_LIBDIRS)

CXXFLAGS =						\
	$(INCDIRS:%=-I%)				\
	$(CEREAL_CXXFLAGS)				\
	-DCEREAL_ENABLE_RAW_POINTER_SERIALIZATION	\
	$(GOOGLETEST_CXXFLAGS)				\
	$(HWLOC_CXXFLAGS)				\
	$(JEMALLOC_CXXFLAGS)				\
	$(QTHREADS_CXXFLAGS)
LDFLAGS =						\
	$(LIBDIRS:%=-L%) $(LIBDIRS:%=-Wl,-rpath,%)	\
	$(CEREAL_LDFLAGS)				\
	$(GOOGLETEST_LDFLAGS)				\
	$(HWLOC_LDFLAGS)				\
	$(JEMALLOC_LDFLAGS)				\
	$(QTHREADS_LDFLAGS)
LIBS =						\
	$(CEREAL_LIBS)				\
	$(GOOGLETEST_LIBS)			\
	$(HWLOC_LIBS)				\
	$(JEMALLOC_LIBS)			\
	$(QTHREADS_LIBS)			\
	$(JEMALLOC_LIBS)

ifneq ($(LLVM_DIR), )

# Try: -fsanitize=address,memory,thread,undefined,vptr
# Cannot have: -fsanitize=alignment,integer,null
# DEBUGFLAGS =					\
	-D_GLIBCXX_DEBUG			\
	-fsanitize-undefined-trap-on-error	\
	-fsanitize=bool				\
	-fsanitize=bounds			\
	-fsanitize=enum				\
	-fsanitize=float-cast-overflow		\
	-fsanitize=integer-divide-by-zero	\
	-fsanitize=nonnull-attribute		\
	-fsanitize=object-size			\
	-fsanitize=return			\
	-fsanitize=returns-nonnull-attribute	\
	-fsanitize=shift			\
	-fsanitize=signed-integer-overflow	\
	-fsanitize=unreachable			\
	-fsanitize=vla-bound
# Note: -flto doesn't see -march=native, hence leads to worse code
OPTFLAGS =					\
	-DNDEBUG				\
	-Ofast					\
	-Wno-unused-variable
CXXFLAGS +=					\
	-Wall					\
	-fmacro-backtrace-limit=0		\
	-ftemplate-backtrace-limit=0		\
	-g					\
	-std=c++14				\
	-Drestrict=__restrict__			\
	-march=native				\
	$(DEBUGFLAGS)				\
	$(OPTFLAGS)

else ifneq ($(GCC_DIR), )

DEBUGFLAGS =					\
	-D_GLIBCXX_DEBUG			\
	-fbounds-check				\
	-fsanitize=undefined			\
	-fstack-protector-all			\
	-ftrapv
# OPTFLAGS =					\
	-DNDEBUG				\
	-Ofast					\
	-flto
CXXFLAGS +=					\
	-Wall					\
	-g					\
	-std=c++14				\
	-m128bit-long-double			\
	-Drestrict=__restrict__			\
	-march=native				\
	$(DEBUGFLAGS)				\
	$(OPTFLAGS)

else

$(error Unknown compiler)

endif

HDRS =						\
	adt/array.hpp				\
	adt/dummy.hpp				\
	adt/either.hpp				\
	adt/empty.hpp				\
	adt/extra.hpp				\
	adt/grid_decl.hpp			\
	adt/grid_impl.hpp			\
	adt/idtype.hpp				\
	adt/maxarray.hpp			\
	adt/maybe.hpp				\
	adt/nested_decl.hpp			\
	adt/nested_impl.hpp			\
	adt/par_decl.hpp			\
	adt/par_impl.hpp			\
	adt/seq_decl.hpp			\
	adt/seq_impl.hpp			\
	adt/tree_decl.hpp			\
	adt/tree_impl.hpp			\
	cxx/apply.hpp				\
	cxx/cstdlib.hpp				\
	cxx/funobj.hpp				\
	cxx/invoke.hpp				\
	cxx/serialize.hpp			\
	cxx/task.hpp				\
	cxx/tuple.hpp				\
	cxx/type_traits.hpp			\
	cxx/utility.hpp				\
	fun/array.hpp				\
	fun/dummy.hpp				\
	fun/either.hpp				\
	fun/empty.hpp				\
	fun/extra.hpp				\
	fun/fun_decl.hpp			\
	fun/fun_impl.hpp			\
	fun/function.hpp			\
	fun/grid_decl.hpp			\
	fun/grid_impl.hpp			\
	fun/idtype.hpp				\
	fun/maxarray.hpp			\
	fun/maybe.hpp				\
	fun/nested_decl.hpp			\
	fun/nested_impl.hpp			\
	fun/pair.hpp				\
	fun/par_decl.hpp			\
	fun/par_impl.hpp			\
	fun/proxy.hpp				\
	fun/seq_decl.hpp			\
	fun/seq_impl.hpp			\
	fun/shared_future.hpp			\
	fun/shared_ptr.hpp			\
	fun/tree_decl.hpp			\
	fun/tree_impl.hpp			\
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
	cxx/serialize.cc
FUNHPC_SRCS =					\
	funhpc/hwloc.cc				\
	funhpc/main.cc				\
	funhpc/server.cc
TEST_SRCS =					\
	adt/array_test.cc			\
	adt/dummy_test.cc			\
	adt/either_test.cc			\
	adt/empty_test.cc			\
	adt/extra_test.cc			\
	adt/grid_test.cc			\
	adt/idtype_test.cc			\
	adt/maxarray_test.cc			\
	adt/maybe_test.cc			\
	adt/nested_test.cc			\
	adt/par_test.cc				\
	adt/seq_test.cc				\
	adt/tree_test.cc			\
	cxx/apply_test.cc			\
	cxx/cstdlib_test.cc			\
	cxx/funobj_test.cc			\
	cxx/invoke_test.cc			\
	cxx/serialize_test.cc			\
	cxx/task_test.cc			\
	cxx/utility_test.cc			\
	fun/array_test.cc			\
	fun/either_test.cc			\
	fun/empty_test.cc			\
	fun/extra_test.cc			\
	fun/fun_test.cc				\
	fun/function_test.cc			\
	fun/grid_test.cc			\
	fun/idtype_test.cc			\
	fun/maxarray_test.cc			\
	fun/maybe_test.cc			\
	fun/nested_test.cc			\
	fun/pair_test.cc			\
	fun/par_test.cc				\
	fun/seq_test.cc				\
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
	examples/benchmark2.cc			\
	examples/fibonacci.cc			\
	examples/hello.cc			\
	examples/pingpong.cc			\
	examples/wave1d.cc			\
	examples/wave3d.cc
HPX_EXAMPLE_SRCS =				\
	examples/benchmark_hpx.cc		\
	examples/hello_hpx.cc
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
$(ALL_SRCS:%.cc=%.o): | format gtest
%.o: %.cc Makefile
	$(MPICXX) -MD $(CXXFLAGS) -c -o $*.o.tmp $*.cc
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
NSHEPHERDS_PER_PROC :=							\
	$(shell echo $$((($(NSHEPHERDS) + $(NPROCS) - 1) / $(NPROCS))))
NTHREADS_PER_PROC :=							\
	$(shell echo $$((($(NTHREADS) + $(NPROCS) - 1) / $(NPROCS))))
NTHREADS_PER_SHEPHERD :=						\
	$(shell echo $$((($(NTHREADS) +					\
	                  $(NPROCS) * $(NSHEPHERDS_PER_PROC) - 1) /	\
		         ($(NPROCS) * $(NSHEPHERDS_PER_PROC)))))

EXE = ./hello
ARGS =
run:
	$(MPIRUN)							   \
	    -np $(NPROCS)						   \
	    -x "QTHREAD_GUARD_PAGES=0"					   \
	    -x "QTHREAD_INFO=1"						   \
	    -x "QTHREAD_NUM_SHEPHERDS=$(NSHEPHERDS_PER_PROC)"		   \
	    -x "QTHREAD_NUM_WORKERS_PER_SHEPHERD=$(NTHREADS_PER_SHEPHERD)" \
	    -x "QTHREAD_STACK_SIZE=65536"				   \
	    $(EXE) $(ARGS)						   \
	    --hpx:info							   \
	    --hpx:ini hpx.stacks.use_guard_pages=0			   \
	    --hpx:numa-sensitive					   \
	    --hpx:threads $(NTHREADS_PER_PROC)
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
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
benchmark2: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/benchmark2.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
fibonacci: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/fibonacci.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
hello: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/hello.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
pingpong: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/pingpong.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
wave1d: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/wave1d.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
wave3d: $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) examples/wave3d.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

### hpx ###

benchmark_hpx: $(SRCS:%.cc=%.o) examples/benchmark_hpx.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
hello_hpx: $(SRCS:%.cc=%.o) examples/hello_hpx.o
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

### check ###

check: selftest selftest-funhpc
	$(MAKE) run EXE=./selftest
	$(MAKE) run NPROCS=2 EXE=./selftest-funhpc
.PHONY: check
selftest: $(TEST_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o) $(GOOGLETEST_MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(GOOGLETEST_OBJS) $(LIBS)
selftest-funhpc:							      \
	$(FUNHPC_TEST_SRCS:%.cc=%.o) $(FUNHPC_SRCS:%.cc=%.o) $(SRCS:%.cc=%.o)
	$(MPICXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(GOOGLETEST_OBJS) $(LIBS)

### external ###

external:
	mkdir external

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
	(cd external &&					\
	    cd $(GOOGLETEST_DIR)/src &&			\
	    $(CXX) $(CXXFLAGS) -c gtest-all.cc &&	\
	    $(CXX) $(CXXFLAGS) -c gtest_main.cc) &&	\
	: > $@
external/gtest.done: external/gtest.built
	: > $@

### clean ###

clean:
	$(RM) $(HDRS:%=%.fmt) $(ALL_SRCS:%=%.fmt)
	$(RM) $(ALL_SRCS:%.cc=%.o) $(ALL_SRCS:%.cc=%.d)
	$(RM) selftest selftest-funhpc
	$(RM) $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%)
	$(RM) wave1d.tsv wave3d.tsv
.PHONY: clean

distclean: clean
	$(RM) -r external $(GOOGLETEST_DIR)
.PHONY: distclean

-include $(ALL_SRCS:%.cc=%.d)
