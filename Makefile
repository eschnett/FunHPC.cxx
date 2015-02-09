# Makefile for FunHPC

GTEST_NAME    = gtest-1.7.0
GTEST_URL     = https://googletest.googlecode.com/files/gtest-1.7.0.zip
GTEST_DIR     = $(abspath ./$(GTEST_NAME))
GTEST_INCDIRS = $(GTEST_DIR)/include $(GTEST_DIR)
GTEST_LIBDIRS = $(GTEST_DIR)/src
GTEST_LIBS    =

HWLOC_NAME    = hwloc-1.10.0
HWLOC_URL     =	http://www.open-mpi.org/software/hwloc/v1.10/downloads/hwloc-1.10.0.tar.bz2
HWLOC_DIR     = $(abspath ./$(HWLOC_NAME))
HWLOC_INCDIRS = $(HWLOC_DIR)/include
HWLOC_LIBDIRS = $(HWLOC_DIR)/lib
HWLOC_LIBS    = hwloc

JEMALLOC_NAME    = jemalloc-3.6.0
JEMALLOC_URL     = http://www.canonware.com/download/jemalloc/jemalloc-3.6.0.tar.bz2
JEMALLOC_DIR     = $(abspath ./$(JEMALLOC_NAME))
JEMALLOC_INCDIRS = $(JEMALLOC_DIR)/include
JEMALLOC_LIBDIRS = $(JEMALLOC_DIR)/lib
JEMALLOC_LIBS    = jemalloc pthread # this must come last

QTHREADS_NAME    = qthread-1.10
QTHREADS_URL     = https://qthreads.googlecode.com/files/qthread-1.10.tar.bz2
QTHREADS_DIR     = $(abspath ./$(QTHREADS_NAME))
QTHREADS_INCDIRS = $(QTHREADS_DIR)/include $(HWLOC_INCDIRS)
QTHREADS_LIBDIRS = $(QTHREADS_DIR)/lib $(HWLOC_LIBDIRS)
QTHREADS_LIBS    = qthread pthread $(HWLOC_LIBS)

INCDIRS = $(GTEST_INCDIRS) $(HWLOC_INCDIRS) $(JEMALLOC_INCDIRS) $(abspath .) $(QTHREADS_INCDIRS)
LIBDIRS = $(GTEST_LIBDIRS) $(HWLOC_LIBDIRS) $(JEMALLOC_LIBDIRS) $(QTHREADS_LIBDIRS)
LIBS    = $(JEMALLOC_LIBS) $(GTEST_LIBS) $(HWLOC_LIBS) $(QTHREADS_LIBS) $(JEMALLOC_LIBS)

# Can also use gcc
CC       = clang
CXX      = clang++
CPPFLAGS = $(INCDIRS:%=-I%)
CFLAGS   = -march=native -Wall -g -std=c99 -Dasm=__asm__
CXXFLAGS = -march=native -Wall -g -std=c++1y -Drestrict=__restrict__
OPTFLAGS = -O3
LDFLAGS  = $(LIBDIRS:%=-L%) $(LIBDIRS:%=-Wl,-rpath,%)

HDRS =	cxx/apply				\
	cxx/invoke				\
	fun/shared_future			\
	fun/shared_ptr				\
	fun/vector				\
	qthread/future				\
	qthread/mutex				\
	qthread/thread
SRCS =	cxx/apply_test.cc			\
	cxx/invoke_test.cc			\
	fun/shared_future_test.cc		\
	fun/shared_ptr_test.cc			\
	fun/vector_test.cc			\
	qthread/future_test.cc			\
	qthread/future_test_std.cc		\
	qthread/mutex_test.cc			\
	qthread/mutex_test_std.cc		\
	qthread/thread_test.cc			\
	qthread/thread_test_std.cc

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

format: $(HDRS:%=%.fmt) $(SRCS:%=%.fmt)
.PHONY: format
%.fmt: % Makefile 
	-clang-format -style=llvm -i $* && : > $*.fmt

### compile ###

objs: $(SRCS:%.cc=%.o)
.PHONY: objs
$(SRCS:%.cc=%.o): | format gtest jemalloc qthreads
%.o: %.cc $(HDRS) Makefile
	$(CXX) -MD $(CPPFLAGS) $(CXXFLAGS) -c -o $*.o.tmp $*.cc
	@$(PROCESS_DEPENDENCIES)
	@mv $*.o.tmp $*.o

### check ###

check: selftest
	env	"LD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		"DYLD_LIBRARY_PATH=$(subst $(space),:,$(LIBDIRS))"	\
		"QTHREAD_STACK_SIZE=65536"				\
		./selftest
.PHONY: check
selftest: $(SRCS:%.cc=%.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -lgtest $(LIBS:%=-l%)

### external ###

external:
	mkdir external

### gtest ###

gtest: external/gtest.done
.PHONY: gtest
external/gtest.downloaded: | external
	(cd external &&				\
		wget $(GTEST_URL)) &&		\
	: > $@
external/gtest.unpacked: external/gtest.downloaded
	rm -rf $(GTEST_NAME) &&				\
	unzip external/$(notdir $(GTEST_URL)) &&	\
	: > $@
external/gtest.built: external/gtest.unpacked
	(cd external &&							       \
		cd $(GTEST_DIR)/src &&					       \
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
	(cd external &&				\
		wget $(JEMALLOC_URL)) &&	\
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
	(cd external &&				\
		wget $(QTHREADS_URL)) &&	\
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
			"CC=$(CC)"				\
			"CXX=$(CXX)"				\
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
	$(RM) $(HDRS:%=%.fmt) $(SRCS:%=%.fmt)
	$(RM) $(SRCS:%.cc=%.o) $(SRCS:%.cc=%.d)
	$(RM) selftest
.PHONY: clean

distclean: clean
	$(RM) -r external $(GTEST_DIR) $(HWLOC_DIR) $(QTHREADS_DIR)
.PHONY: distclean

-include $(SRCS:%.cc=%.d)
