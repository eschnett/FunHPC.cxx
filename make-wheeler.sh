#!/bin/bash

SPACK_DIR="~eschnett/src/spack-view"

CXX="$SPACK_DIR/bin/g++"
MPICXX="$SPACK_DIR/bin/mpicxx"
MPIRUN="$SPACK_DIR/bin/mpiexec"

make "$@"                                       \
    CEREAL_DIR="$SPACK_DIR"                     \
    HWLOC_DIR="$SPACK_DIR"                      \
    JEMALLOC_DIR="$SPACK_DIR"                   \
    QTHREADS_DIR="$SPACK_DIR"                   \
    CXX="$CXX"                                  \
    MPICXX="$MPICXX"                            \
    MPIRUN="$MPIRUN"
