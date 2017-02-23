# CMake setup

cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0048 NEW)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

project(FunHPC VERSION 1.0.0 LANGUAGES C CXX)
set(PROJECT_DESCRIPTION "FunHPC: Functional HPC Programming")
set(PROJECT_URL "https://bitbucket.org/eschnett/funhpc.cxx")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")

# External dependencies

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Drestrict=__restrict__")

find_package(Cereal REQUIRED)
include_directories(${CEREAL_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCEREAL_ENABLE_RAW_POINTER_SERIALIZATION")
set(LIBS ${LIBS} ${CEREAL_LIBRARIES})

find_package(Hwloc REQUIRED)
include_directories(${HWLOC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${HWLOC_LIBRARIES})

find_package(Jemalloc REQUIRED)
include_directories(${JEMALLOC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${JEMALLOC_LIBRARIES})

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_DIRS})
set(LIBS ${LIBS} ${MPI_LIBRARIES})

find_package(Qthread REQUIRED)
include_directories(${QTHREAD_INCLUDE_DIRS})
set(LIBS ${LIBS} ${QTHREAD_LIBRARIES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(LIBS ${LIBS} Threads::Threads)

# Main project

configure_file(
  "${PROJECT_SOURCE_DIR}/funhpc/config.hpp.in"
  "${PROJECT_BINARY_DIR}/funhpc/config.hpp"
  )

include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_BINARY_DIR}"
  )

set(SRCS
  cxx/cstdlib.cpp
  cxx/serialize.cpp
  funhpc/config.cpp
  )
set(FUNHPC_SRCS
  funhpc/hwloc.cpp
  funhpc/main.cpp
  funhpc/server.cpp
  )

add_library(funhpc ${SRCS} ${FUNHPC_SRCS})
target_link_libraries(funhpc ${LIBS})
set_property(TARGET funhpc PROPERTY CXX_STANDARD 14)

# Self-tests

set(GOOGLETEST_DIR "googletest-release-1.8.0/googletest")
include_directories("${GOOGLETEST_DIR}/include" "${GOOGLETEST_DIR}")

set(TEST_SRCS
  adt/arith_test.cpp
  adt/dummy_test.cpp
  adt/either_test.cpp
  adt/empty_test.cpp
  adt/extra_test.cpp
  adt/grid_test.cpp
  adt/grid2_test.cpp
  adt/idtype_test.cpp
  adt/index_test.cpp
  adt/maxarray_test.cpp
  adt/maybe_test.cpp
  adt/nested_test.cpp
  adt/par_test.cpp
  adt/seq_test.cpp
  adt/tree_test.cpp
  cxx/apply_test.cpp
  cxx/cstdlib_test.cpp
  cxx/funobj_test.cpp
  cxx/invoke_test.cpp
  cxx/serialize_test.cpp
  cxx/task_test.cpp
  cxx/utility_test.cpp
  fun/array_test.cpp
  fun/either_test.cpp
  fun/empty_test.cpp
  fun/extra_test.cpp
  fun/fun_test.cpp
  fun/function_test.cpp
  fun/grid2_test.cpp
  fun/grid_test.cpp
  fun/idtype_test.cpp
  fun/maxarray_test.cpp
  fun/maybe_test.cpp
  fun/nested_test.cpp
  fun/pair_test.cpp
  fun/par_test.cpp
  fun/seq_test.cpp
  fun/shared_future_test.cpp
  fun/shared_ptr_test.cpp
  fun/tree_test.cpp
  fun/vector_test.cpp
  funhpc/config_test.cpp
  qthread/future_test.cpp
  qthread/future_test_std.cpp
  qthread/mutex_test.cpp
  qthread/mutex_test_std.cpp
  qthread/thread_test.cpp
  qthread/thread_test_std.cpp
  )

set(FUNHPC_TEST_SRCS
  fun/proxy_test.cpp
  funhpc/async_test.cpp
  funhpc/proxy_test.cpp
  funhpc/rexec_test.cpp
  funhpc/server_test.cpp
  funhpc/shared_rptr_test.cpp
  funhpc/test_main.cpp
  )

add_executable(selftest ${SRCS} ${TEST_SRCS} "${GOOGLETEST_DIR}/src/gtest-all.cc" "${GOOGLETEST_DIR}/src/gtest_main.cc")
set_property(TARGET selftest PROPERTY CXX_STANDARD 14)
target_link_libraries(selftest funhpc)

add_executable(selftest-funhpc ${SRCS} ${FUNHPC_TEST_SRCS} "${GOOGLETEST_DIR}/src/gtest-all.cc")
set_property(TARGET selftest-funhpc PROPERTY CXX_STANDARD 14)
target_link_libraries(selftest-funhpc funhpc)

enable_testing()
add_test(NAME selftest COMMAND ./selftest)
add_test(NAME selftest-funhpc
  COMMAND
  env
  QTHREAD_NUM_SHEPHERDS=1
  QTHREAD_NUM_WORKERS_PER_SHEPHERD=2
  QTHREAD_STACK_SIZE=65536
  FUNHPC_NUM_NODES=1
  FUNHPC_NUM_PROCS=2
  FUNHPC_NUM_THREADS=2
  ${MPIEXEC}
  ${MPIEXEC_NUMPROC_FLAG} 2
  ${MPIEXEC_PREFLAGS}
  ./selftest-funhpc
  ${MPIEXEC_POSTFLAGS})

# Examples

add_executable(benchmark EXCLUDE_FROM_ALL examples/benchmark.cpp)
set_property(TARGET benchmark PROPERTY CXX_STANDARD 14)
target_link_libraries(benchmark funhpc)

add_executable(benchmark2 EXCLUDE_FROM_ALL examples/benchmark2.cpp)
set_property(TARGET benchmark2 PROPERTY CXX_STANDARD 14)
target_link_libraries(benchmark2 funhpc)

add_executable(fibonacci EXCLUDE_FROM_ALL examples/fibonacci.cpp)
set_property(TARGET fibonacci PROPERTY CXX_STANDARD 14)
target_link_libraries(fibonacci funhpc)

add_executable(hello EXCLUDE_FROM_ALL examples/hello.cpp funhpc)
set_property(TARGET hello PROPERTY CXX_STANDARD 14)
target_link_libraries(hello funhpc)

add_executable(loops EXCLUDE_FROM_ALL examples/loops.cpp)
set_property(TARGET loops PROPERTY CXX_STANDARD 14)
target_link_libraries(loops funhpc)

add_executable(pingpong EXCLUDE_FROM_ALL examples/pingpong.cpp)
set_property(TARGET pingpong PROPERTY CXX_STANDARD 14)
target_link_libraries(pingpong funhpc)

add_executable(wave1d EXCLUDE_FROM_ALL examples/wave1d.cpp)
set_property(TARGET wave1d PROPERTY CXX_STANDARD 14)
target_link_libraries(wave1d funhpc)

add_executable(wave3d EXCLUDE_FROM_ALL examples/wave3d.cpp)
set_property(TARGET wave3d PROPERTY CXX_STANDARD 14)
target_link_libraries(wave3d funhpc)

add_custom_target(examples EXCLUDE_FROM_ALL
  DEPENDS
  benchmark
  benchmark2
  fibonacci
  hello
  loops
  pingpong
  wave1d
  wave3d
  )

# Install

set(HEADERS
  adt/arith.hpp
  adt/dummy.hpp
  adt/either.hpp
  adt/empty.hpp
  adt/extra.hpp
  adt/grid_decl.hpp
  adt/grid_impl.hpp
  adt/grid2_decl.hpp
  adt/grid2_impl.hpp
  adt/idtype.hpp
  adt/index.hpp
  adt/maxarray.hpp
  adt/maybe.hpp
  adt/nested_decl.hpp
  adt/nested_impl.hpp
  adt/par_decl.hpp
  adt/par_impl.hpp
  adt/seq_decl.hpp
  adt/seq_impl.hpp
  adt/tree_decl.hpp
  adt/tree_impl.hpp
  cxx/apply.hpp
  cxx/cassert.hpp
  cxx/cstdlib.hpp
  cxx/funobj.hpp
  cxx/invoke.hpp
  cxx/serialize.hpp
  cxx/task.hpp
  cxx/tuple.hpp
  cxx/type_traits.hpp
  cxx/utility.hpp
  fun/array.hpp
  fun/dummy.hpp
  fun/either.hpp
  fun/empty.hpp
  fun/extra.hpp
  fun/fun_decl.hpp
  fun/fun_impl.hpp
  fun/function.hpp
  fun/grid2_decl.hpp
  fun/grid2_impl.hpp
  fun/grid_decl.hpp
  fun/grid_impl.hpp
  fun/idtype.hpp
  fun/maxarray.hpp
  fun/maybe.hpp
  fun/nested_decl.hpp
  fun/nested_impl.hpp
  fun/pair.hpp
  fun/par_decl.hpp
  fun/par_impl.hpp
  fun/proxy.hpp
  fun/seq_decl.hpp
  fun/seq_impl.hpp
  fun/shared_future.hpp
  fun/shared_ptr.hpp
  fun/tree_decl.hpp
  fun/tree_impl.hpp
  fun/vector.hpp
  funhpc/async.hpp
  funhpc/hwloc.hpp
  funhpc/main.hpp
  funhpc/proxy.hpp
  funhpc/rexec.hpp
  funhpc/rptr.hpp
  funhpc/serialize_shared_future.hpp
  funhpc/server.hpp
  funhpc/shared_rptr.hpp
  qthread/future.hpp
  qthread/mutex.hpp
  qthread/thread.hpp
  )

foreach(DIR adt cxx fun funhpc qthread)
  set(FILES ${HEADERS})
  list(FILTER FILES INCLUDE REGEX "^${DIR}/.*")
  install(FILES ${FILES} DESTINATION include/${DIR})
endforeach()

install(FILES "${PROJECT_BINARY_DIR}/funhpc/config.hpp" DESTINATION include/funhpc)

install(TARGETS funhpc DESTINATION lib)

# Don't install self-tests or examples. If we do, we need to rename
# them (e.g. by adding a "funhpc-" prefix) to avoid name clashes.

set(PKG_CONFIG_REQUIRES "cereal hwloc jemalloc mpi qthread")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/funhpc")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")
set(PKG_CONFIG_LIBS "-L\${libdir} -lfunhpc")

configure_file(
  "${PROJECT_SOURCE_DIR}/pkg-config.pc.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
