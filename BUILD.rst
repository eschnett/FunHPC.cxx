::
   (rm -rf build && mkdir build && cd build && env PATH="$HOME/src/spack-view/bin:$PATH" cmake -G Ninja -DCMAKE_CXX_COMPILER="$HOME/src/spack-view/bin/g++" -DCMAKE_CXX_FLAGS="-march=native" -DCMAKE_FIND_ROOT_PATH="$HOME/spack-view" -DGTEST_ROOT="$HOME/src/spack-view" ..)
   pushd build
   ninja
   ninja hello && env FUNHPC_NUM_NODES=1 FUNHPC_NUM_PROCS=1 FUNHPC_NUM_THREADS=8 QTHREAD_NUM_SHEPHERDS=1 QTHREAD_NUM_WORKERS_PER_SHEPHERD=8 $HOME/src/spack-view/bin/mpirun -np 1 ./hello 
   popd
