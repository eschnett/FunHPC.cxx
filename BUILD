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



for f in job-n*.s*.c*.p*.t*.m*.r*.log; do grep -H 'Time:' $f | tail -n +3; done



===============================================================================

10   wave-vector
20   wave
30   wave3d-grid
40   wave3d-tree
50   wave3d-grid 2d
60   wave3d-tree 2d

{
for r in 10; do for n in 1 2 4 8; do for t in 8; do ./submit.mike.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave-vector& done; done; done
for r in 20; do for n in 1 2 4 8; do for t in 8; do ./submit.mike.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave& done; done; done
for r in 30; do for n in 1 2 4 8; do for t in 8; do ./submit.mike.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 40; do for n in 1 2 4 8; do for t in 8; do ./submit.mike.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done
}; wait

for r in 10 20 30 40 50 60; do for t in 8; do for n in 1 2 4 8 16 32 64 128; do for f in job-n$n.s*.c*.p*.t$t.m*.r$r.log; do grep -H 'Time:' $f | tail -n +3 | head -n 1; done; done; done; done

wave-vector:
job-n1.s2.c8.p2.t8.m1.r10.log:       Time:  12.3746 sec
job-n2.s2.c8.p4.t8.m1.r10.log:       Time:  21.3313 sec
job-n4.s2.c8.p8.t8.m1.r10.log:       Time:  99.7892 sec
job-n8.s2.c8.p16.t8.m1.r10.log:      Time: 229.932 sec

wave:
job-n1.s2.c8.p2.t8.m1.r20.log:       Time:   2.14479 sec
job-n2.s2.c8.p4.t8.m1.r20.log:       Time:   1.76385 sec
job-n4.s2.c8.p8.t8.m1.r20.log:       Time:   4.41958 sec
job-n8.s2.c8.p16.t8.m1.r20.log:      Time:   2.95982 sec
job-n16.s2.c8.p32.t8.m1.r20.log:     Time:   5.19773 sec
job-n32.s2.c8.p64.t8.m1.r20.log:     Time:   8.58659 sec
job-n64.s2.c8.p128.t8.m1.r20.log:    Time:  13.5954 sec
job-n128.s2.c8.p256.t8.m1.r20.log:   Time:   8.20434 sec

wave3d-grid:
job-n1.s2.c8.p2.t8.m1.r30.log:       Time:   0.263105 sec
job-n2.s2.c8.p4.t8.m1.r30.log:       Time:   0.52935 sec
job-n4.s2.c8.p8.t8.m1.r30.log:       Time:   1.05167 sec
job-n8.s2.c8.p16.t8.m1.r30.log:      Time:   2.11658 sec
job-n16.s2.c8.p32.t8.m1.r30.log:     Time:   4.28292 sec
job-n32.s2.c8.p64.t8.m1.r30.log:     Time:   8.60039 sec
job-n64.s2.c8.p128.t8.m1.r30.log:    Time:  17.2712 sec
job-n128.s2.c8.p256.t8.m1.r30.log:   Time:  33.7294 sec

wave3d-tree:
job-n1.s2.c8.p2.t8.m1.r40.log:       Time:   1.72825 sec
job-n2.s2.c8.p4.t8.m1.r40.log:       Time:   1.07252 sec
job-n4.s2.c8.p8.t8.m1.r40.log:       Time:   3.01409 sec
job-n8.s2.c8.p16.t8.m1.r40.log:      Time:   1.42693 sec
job-n16.s2.c8.p32.t8.m1.r40.log:     Time:   2.95823 sec
job-n32.s2.c8.p64.t8.m1.r40.log:     Time:   7.60146 sec
job-n64.s2.c8.p128.t8.m1.r40.log:    Time:  10.5571 sec
job-n128.s2.c8.p256.t8.m1.r40.log:   Time:   7.69732 sec

wave3d-grid 2d:
job-n1.s2.c8.p2.t8.m1.r50.log:       Time:   0.440844 sec
job-n2.s2.c8.p4.t8.m1.r50.log:       Time:   0.874157 sec
job-n4.s2.c8.p8.t8.m1.r50.log:       Time:   1.77445 sec
job-n8.s2.c8.p16.t8.m1.r50.log:      Time:   3.63771 sec
