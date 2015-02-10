# Makefile for FunHPC

CEREAL_NAME     = cereal-1.1.0
CEREAL_URL      = https://github.com/USCiLab/cereal/archive/v1.1.0.tar.gz
CEREAL_DIR      = $(abspath ./$(CEREAL_NAME))
CEREAL_CPPFLAGS = -DCEREAL_ENABLE_RAW_POINTER_SERIALIZATION
CEREAL_INCDIRS  = $(CEREAL_DIR)/include
CEREAL_LIBDIRS  =
CEREAL_LIBS     =

GOOGLETEST_NAME     = gtest-1.7.0
GOOGLETEST_URL      = https://googletest.googlecode.com/files/gtest-1.7.0.zip
GOOGLETEST_DIR      = $(abspath ./$(GOOGLETEST_NAME))
GOOGLETEST_CPPFLAGS =
GOOGLETEST_INCDIRS  = $(GOOGLETEST_DIR)/include $(GOOGLETEST_DIR)
GOOGLETEST_LIBDIRS  = $(GOOGLETEST_DIR)/src
GOOGLETEST_LIBS     =

HWLOC_NAME     = hwloc-1.10.0
HWLOC_URL      = http://www.open-mpi.org/software/hwloc/v1.10/downloads/hwloc-1.10.0.tar.bz2
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

QTHREADS_NAME     = qthread-1.10
QTHREADS_URL      = https://qthreads.googlecode.com/files/qthread-1.10.tar.bz2
QTHREADS_DIR      = $(abspath ./$(QTHREADS_NAME))
QTHREADS_CPPFLAGS =
QTHREADS_INCDIRS  = $(QTHREADS_DIR)/include $(HWLOC_INCDIRS)
QTHREADS_LIBDIRS  = $(QTHREADS_DIR)/lib $(HWLOC_LIBDIRS)
QTHREADS_LIBS     = qthread pthread $(HWLOC_LIBS)

INCDIRS = $(CEREAL_INCDIRS) $(GOOGLETEST_INCDIRS) $(HWLOC_INCDIRS) $(JEMALLOC_INCDIRS) $(abspath .) $(QTHREADS_INCDIRS)
LIBDIRS = $(CEREAL_LIBDIRS) $(GOOGLETEST_LIBDIRS) $(HWLOC_LIBDIRS) $(JEMALLOC_LIBDIRS) $(QTHREADS_LIBDIRS)
LIBS    = $(CEREAL_LIBS) $(GOOGLETEST_LIBS) $(HWLOC_LIBS) $(QTHREADS_LIBS) $(JEMALLOC_LIBS)

# Can also use gcc
CC          = clang
CXX         = clang++
CPPFLAGS    = $(INCDIRS:%=-I%) $(CEREAL_CPPFLAGS) $(GOOGLETEST_CPPFLAGS) $(HWLOC_CPPFLAGS) $(JEMALLOC_CPPFLAGS) $(QTHREADS_CPPFLAGS)
CFLAGS      = -march=native -Wall -g -std=c99 -Dasm=__asm__
CXXFLAGS    = -march=native -Wall -g -std=c++1y -Drestrict=__restrict__
OPTFLAGS    = -O3
LDFLAGS     = $(LIBDIRS:%=-L%) $(LIBDIRS:%=-Wl,-rpath,%)

MPICC       = env "OMPI_CC=$(CC)" mpicc
MPICXX      = env "OMPI_CXX=$(CXX)" mpicxx
MPICPPFLAGS = $(CPPFLAGS)
MPICFLAGS   = $(CFLAGS)
MPICXXFLAGS = $(CXXFLAGS)
MPILDFLAGS  = $(LDFLAGS)
MPILIBS     = $(LIBS)
MPIRUN      = mpirun

HDRS =	cxx/apply				\
	cxx/invoke				\
	cxx/serialize				\
	cxx/task				\
	fun/shared_future			\
	fun/shared_ptr				\
	fun/vector				\
	funhpc/main				\
	funhpc/rexec				\
	funhpc/server				\
	qthread/future				\
	qthread/mutex				\
	qthread/thread
FUNHPC_SRCS =					\
	funhpc/main_impl.cc			\
	funhpc/server_impl.cc
TEST_SRCS =					\
	cxx/apply_test.cc			\
	cxx/invoke_test.cc			\
	cxx/serialize_test.cc			\
	cxx/task_test.cc			\
	fun/shared_future_test.cc		\
	fun/shared_ptr_test.cc			\
	fun/vector_test.cc			\
	qthread/future_test.cc			\
	qthread/future_test_std.cc		\
	qthread/mutex_test.cc			\
	qthread/mutex_test_std.cc		\
	qthread/thread_test.cc			\
	qthread/thread_test_std.cc
FUNHPC_TEST_SRCS =				\
	funhpc/rexec_test.cc			\
	funhpc/test_main.cc
FUNHPC_EXAMPLE_SRCS =				\
	examples/hello.cc			\
	examples/million.cc			\
	examples/pingpong.cc
ALL_SRCS = $(SRCS) $(FUNHPC_SRCS) $(TEST_SRCS) $(FUNHPC_TEST_SRCS) $(FUNHPC_EXAMPLE_SRCS)

# Taken from <http://mad-scientist.net/make/autodep.html> as written
# by Paul D. Smith <psmith@gnu.org>, originally developed by Tom
# Tromey <tromey@cygnus.com>
PROCESS_DEPENDENCIES =							      \
	{								      \
	perl -p -e 's{$*.o.tmp}{$*.o}g' < $*.o.d &&			      \
	perl -p -e 's{\#.*}{};s{^[^:]*: *}{};s{ *\\$$}{};s{$$}{ :}' < $*.o.d; \
	} > $*.d &&							      \
	rm -f $*.o.d

comma := ,
empty :=
space := $(empty) $(empty)

all: format objs
.PHONY: all

### format ###

format: $(HDRS:%=%.fmt) $(ALL_SRCS:%=%.fmt)
.PHONY: format
%.fmt: % Makefile 
	-clang-format -style=llvm -i $* && : > $*.fmt

### compile ###

objs: $(ALL_SRCS:%.cc=%.o)
.PHONY: objs
$(ALL_SRCS:%.cc=%.o): | format cereal gtest jemalloc hwloc qthreads
%.o: %.cc $(HDRS) Makefile
	$(MPICXX) -MD $(MPICPPFLAGS) $(MPICXXFLAGS) -c -o $*.o.tmp $*.cc
	@$(PROCESS_DEPENDENCIES)
	@mv $*.o.tmp $*.o

### examples ###

examples: $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%)
	for example in $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%); do	   \
		env	"LD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"   \
			"DYLD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))" \
			"QTHREAD_STACK_SIZE=65536"			   \
			./$$example;					   \
	done
	for example in $(FUNHPC_EXAMPLE_SRCS:examples/%.cc=%); do	      \
		$(MPIRUN) -np 2						      \
			-x "LD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"   \
			-x "DYLD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))" \
			-x "QTHREAD_STACK_SIZE=65536"			      \
			./$$example;					      \
	done
.PHONY: examples
hello: $(FUNHPC_SRCS:%.cc=%.o) examples/hello.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
		$(MPILIBS:%=-l%)
million: $(FUNHPC_SRCS:%.cc=%.o) examples/million.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
		$(MPILIBS:%=-l%)
pingpong: $(FUNHPC_SRCS:%.cc=%.o) examples/pingpong.o
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^	\
		$(MPILIBS:%=-l%)

### check ###

check: selftest selftest-funhpc
	env	"LD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		"DYLD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		"QTHREAD_STACK_SIZE=65536"				\
		./selftest
	$(MPIRUN) -np 2							\
		-x "LD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		-x "DYLD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		-x "QTHREAD_STACK_SIZE=65536"				\
		./selftest-funhpc
.PHONY: check
selftest: $(TEST_SRCS:%.cc=%.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -lgtest $(LIBS:%=-l%)
selftest-funhpc: $(FUNHPC_TEST_SRCS:%.cc=%.o) $(FUNHPC_SRCS:%.cc=%.o)
	$(MPICXX) $(MPICPPFLAGS) $(MPICXXFLAGS) $(MPILDFLAGS) -o $@ $^ -lgtest $(MPILIBS:%=-l%)

### external ###

external:
	mkdir external

### Cereal ###

cereal: external/cereal.done
.PHONY: cereal
external/cereal.downloaded: | external
	(cd external &&					\
		$(RM) $(notdir $(CEREAL_URL)) &&	\
		wget $(CEREAL_URL)) &&			\
	: > $@
external/cereal.unpacked: external/cereal.downloaded
	rm -rf $(CEREAL_NAME) &&			\
	tar xzf external/$(notdir $(CEREAL_URL)) &&	\
	(cd $(CEREAL_NAME) &&				\
		patch -p0 < $(abspath cereal.patch)) &&	\
	: > $@
external/cereal.done: external/cereal.unpacked
	: > $@

### Google Test ###

gtest: external/gtest.done
.PHONY: gtest
external/gtest.downloaded: | external
	(cd external &&				\
		$(RM) $(notdir $(GOOGLETEST_URL)) &&	\
		wget $(GOOGLETEST_URL)) &&		\
	: > $@
external/gtest.unpacked: external/gtest.downloaded
	rm -rf $(GOOGLETEST_NAME) &&				\
	unzip external/$(notdir $(GOOGLETEST_URL)) &&	\
	: > $@
external/gtest.built: external/gtest.unpacked
	(cd external &&							       \
		cd $(GOOGLETEST_DIR)/src &&					       \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OPTFLAGS) -c gtest-all.cc &&  \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(OPTFLAGS) -c gtest_main.cc && \
		$(AR) -r -c libgtest.a gtest-all.o gtest_main.o) &&	       \
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
	(cd external &&					\
		rm -rf $(HWLOC_NAME) &&			\
		tar xjf $(notdir $(HWLOC_URL))) &&	\
	: > $@
external/hwloc.built: external/hwloc.unpacked
	+(cd external &&					\
		rm -rf $(HWLOC_NAME)-build &&			\
		mkdir $(HWLOC_NAME)-build &&			\
		cd $(HWLOC_NAME)-build &&			\
		"$(abspath external/$(HWLOC_NAME)/configure)"	\
			--prefix="$(HWLOC_DIR)"			\
			--disable-libxml2			\
			"CC=$(CC)"				\
			"CXX=$(CXX)"				\
			"CFLAGS=$(CFLAGS) $(OPTFLAGS)"		\
			"CXXFLAGS=$(CXXFLAGS) $(OPTFLAGS)" &&	\
		$(MAKE)) &&					\
	: > $@
external/hwloc.installed: external/hwloc.built
	+(cd external &&			\
		rm -rf $(HWLOC_DIR) &&		\
		cd $(HWLOC_NAME)-build &&	\
		$(MAKE) install) &&		\
	: > $@
external/hwloc.done: external/hwloc.installed
	: > $@

### jemalloc ###

jemalloc: external/jemalloc.done
.PHONY: jemalloc
external/jemalloc.downloaded: | external
	(cd external &&					\
		$(RM) $(notdir $(JEMALLOC_URL)) &&	\
		wget $(JEMALLOC_URL)) &&		\
	: > $@
external/jemalloc.unpacked: external/jemalloc.downloaded
	(cd external &&					\
		rm -rf $(JEMALLOC_NAME) &&		\
		tar xjf $(notdir $(JEMALLOC_URL))) &&	\
	: > $@
external/jemalloc.built: external/jemalloc.unpacked
	+(cd external &&						\
		rm -rf $(JEMALLOC_NAME)-build &&			\
		mkdir $(JEMALLOC_NAME)-build &&				\
		cd $(JEMALLOC_NAME)-build &&				\
		"$(abspath external/$(JEMALLOC_NAME)/configure)"	\
			--prefix="$(JEMALLOC_DIR)"			\
			"CC=$(CC)"					\
			"CXX=$(CXX)"					\
			"CFLAGS=$(CFLAGS) $(OPTFLAGS)"			\
			"CXXFLAGS=$(CXXFLAGS) $(OPTFLAGS)" &&		\
		$(MAKE)) &&						\
	: > $@
external/jemalloc.installed: external/jemalloc.built
	+(cd external &&			\
		rm -rf $(JEMALLOC_DIR) &&	\
		cd $(JEMALLOC_NAME)-build &&	\
		$(MAKE) install) &&		\
	: > $@
external/jemalloc.done: external/jemalloc.installed
	: > $@

### Qthreads ###

qthreads: external/qthreads.done
.PHONY: qthreads
external/qthreads.downloaded: | external
	(cd external &&					\
		$(RM) $(notdir $(QTHREADS_URL)) &&	\
		wget $(QTHREADS_URL)) &&		\
	: > $@
external/qthreads.unpacked: external/qthreads.downloaded
	(cd external &&						\
		rm -rf $(QTHREADS_NAME) &&			\
		tar xjf $(notdir $(QTHREADS_URL)) &&		\
		cd $(QTHREADS_NAME) &&				\
		patch -p0 < $(abspath qthreads.patch)) &&	\
	: > $@
external/qthreads.built: external/qthreads.unpacked | hwloc
	+(cd external &&						\
		rm -rf $(QTHREADS_NAME)-build &&			\
		mkdir $(QTHREADS_NAME)-build &&				\
		cd $(QTHREADS_NAME)-build &&				\
		"$(abspath external/$(QTHREADS_NAME)/configure)"	\
			--prefix="$(QTHREADS_DIR)"			\
			--enable-guard-pages --enable-debug=yes		\
			--with-hwloc="$(HWLOC_DIR)"			\
			"CC=$(CC)"					\
			"CXX=$(CXX)"					\
			"CFLAGS=$(CFLAGS) $(OPTFLAGS)"			\
			"CXXFLAGS=$(CXXFLAGS) $(OPTFLAGS)" &&		\
		$(MAKE)) &&						\
	: > $@
external/qthreads.installed: external/qthreads.built
	+(cd external &&			\
		rm -rf $(QTHREADS_DIR) &&	\
		cd $(QTHREADS_NAME)-build &&	\
		$(MAKE) install) &&		\
	: > $@
external/qthreads.done: external/qthreads.installed
	: > $@

### clean ###

clean:
	$(RM) $(HDRS:%=%.fmt) $(ALL_SRCS:%=%.fmt)
	$(RM) $(ALL_SRCS:%.cc=%.o) $(ALL_SRCS:%.cc=%.d)
	$(RM) $(FUNHPC_gEXAMPLE_SRCS:examples/%.cc=%)
	$(RM) selftest
.PHONY: clean

distclean: clean
	$(RM) -r external $(CEREAL_DIR) $(GOOGLETEST_DIR) $(HWLOC_DIR)	\
		$(JEMALLOC_DIR) $(QTHREADS_DIR)
.PHONY: distclean

-include $(ALL_SRCS:%.cc=%.d)
