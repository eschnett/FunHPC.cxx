# Makefile for FunHPC

GTEST_NAME    = gtest-1.7.0
GTEST_URL     = https://googletest.googlecode.com/files/$(GTEST_NAME).zip
GTEST_SRCS    = $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc
GTEST_DIR     = $(GTEST_NAME)
GTEST_INCDIRS = $(GTEST_DIR)/include $(GTEST_DIR)
GTEST_LIBDIRS =
GTEST_LIBS    =

QTHREADS_NAME    = qthread-1.10
QTHREADS_URL     =							\
	https://qthreads.googlecode.com/files/$(QTHREADS_NAME).tar.bz2
QTHREADS_DIR     = $(QTHREADS_NAME)
QTHREADS_INCDIRS = $(QTHREADS_DIR)-install/include
QTHREADS_LIBDIRS = $(QTHREADS_DIR)-install/lib
QTHREADS_LIBS    = qthread

INCDIRS = $(GTEST_INCDIRS) $(QTHREADS_INCDIRS) .
LIBDIRS = $(GTEST_LIBDIRS) $(QTHREADS_LIBDIRS)
LIBS    = $(GTEST_LIBS) $(QTHREADS_LIBS)

# Can also use g++
CXX      = clang++
CPPFLAGS = -Drestrict=__restrict__ $(INCDIRS:%=-I%)
CXXFLAGS = -std=c++14 -march=native -Wall -g
LDFLAGS  = $(LIBDIRS:%=-L%)

HDRS =	cxx/apply cxx/invoke \
	qthread/future qthread/mutex qthread/thread
SRCS =	cxx/apply_test.cc cxx/invoke_test.cc \
	qthread/future_test.cc qthread/mutex_test.cc \
	qthread/thread_test.cc

# Taken from <http://mad-scientist.net/make/autodep.html> as written
# by Paul D. Smith <psmith@gnu.org>, originally developed by Tom
# Tromey <tromey@cygnus.com>
PROCESS_DEPENDENCIES =							      \
	{								      \
	perl -p -e 's{$*.o.tmp}{$*.o}g' < $*.o.d &&			      \
	perl -p -e 's{\#.*}{};s{^[^:]*: *}{};s{ *\\$$}{};s{$$}{ :}' < $*.o.d; \
	} > $*.d &&							      \
	rm -f $*.o.d

all: format objs
.PHONY: all

format: $(HDRS:%=%.fmt) $(SRCS:%=%.fmt)
.PHONY: format

%.fmt: % Makefile 
	clang-format -style=llvm -i $* && : > $*.fmt

objs: $(SRCS:%.cc=%.o)
.PHONY: objs
%.o: %.cc $(HDRS) Makefile | format $(GTEST_DIR).src $(QTHREADS_DIR).installed
	$(CXX) -MD $(CPPFLAGS) $(CXXFLAGS) -c -o $*.o.tmp $*.cc
	@$(PROCESS_DEPENDENCIES)
	@mv $*.o.tmp $*.o

check: selftest
	./selftest
.PHONY: check
selftest: $(SRCS:%.cc=%.o) $(GTEST_SRCS:%.cc=%.o)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS:%=-l%)

### gtest ###

$(GTEST_NAME).zip:
	wget -O $@.tmp $(GTEST_URL) && mv $@.tmp $@
$(GTEST_DIR).src: $(GTEST_NAME).zip
	$(RM) -r $(GTEST_DIR)
	unzip $^ && : > $@

### Qthreads ###

$(QTHREADS_NAME).tar.bz2:
	wget -O $@.tmp $(QTHREADS_URL) && mv $@.tmp $@
$(QTHREADS_DIR).installed: $(QTHREADS_NAME).tar.bz2
	+env "MAKE=$(MAKE)"				\
		"CXX=$(CXX)"				\
		"CXXFLAGS=$(CXXFLAGS)"			\
		"QTHREADS_NAME=$(QTHREADS_NAME)"	\
		"QTHREADS_DIR=$(QTHREADS_DIR)"		\
		./build-qthreads.sh &&			\
	: > $@

clean:
	$(RM) $(HDRS:%=%.fmt) $(SRCS:%=%.fmt)
	$(RM) $(SRCS:%.cc=%.o) $(SRCS:%.cc=%.d)
	$(RM) $(GTEST_SRCS:%.cc=%.o) $(GTEST_SRCS:%.cc=%.d)
	$(RM) selftest
.PHONY: clean

distclean: clean
	$(RM) $(GTEST_NAME).zip
	$(RM) -r $(GTEST_DIR) $(GTEST_DIR).src
.PHONY: distclean

-include $(SRCS:%.cc=%.d)
