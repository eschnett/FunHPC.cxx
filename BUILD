mosh --server=\$HOME/gordon/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@gordon.sdsc.edu

mosh --server=\$HOME/work/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server schnette@hopper.nersc.gov

mosh eschnetter@nvidia

NOmosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@shelob.hpc.lsu.edu

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@stampede.tacc.utexas.edu

mosh --server=\$HOME/trestles/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@trestles.sdsc.edu



rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@login-damiana.aei.mpg.de:datura/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit ux452368@gordon.sdsc.edu:gordon/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit schnette@hopper.nersc.gov:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@mike.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnetter@nvidia.pi.local:/xfs1/eschnetter/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@shelob.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@stampede.tacc.utexas.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit ux452368@trestles.sdsc.edu:trestles/src/mpi-rpc/



(source $HOME/SIMFACTORY/cereal-1.0.0/env.sh && source $HOME/SIMFACTORY/hwloc-1.10.0/env.sh && source $HOME/SIMFACTORY/jemalloc-3.6.0/env.sh && source $HOME/SIMFACTORY/llvm-3.5.0/env.sh && source $HOME/SIMFACTORY/openmpi-1.8.3/env.sh && source $HOME/SIMFACTORY/qthreads-1.10/env.sh && make -j8 wave)

(export SIMFACTORY_SIM=$HOME/Cbeta/simfactory3/sim && source $HOME/SIMFACTORY/cereal-1.0.0/env.sh && source $HOME/SIMFACTORY/hwloc-1.10.0/env.sh && source $HOME/SIMFACTORY/jemalloc-3.6.0/env.sh && source $HOME/SIMFACTORY/llvm-3.5.0/env.sh && source $HOME/SIMFACTORY/openmpi-1.8.3/env.sh && source $HOME/SIMFACTORY/qthreads-1.10/env.sh && make -j8 format && make -j8 wave)



for f in job.n*.s*.c*.p*.t*.m*.r*.log; do grep -H 'Time:' $f | tail -n +3; done
