mosh --server=\$HOME/gordon/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@gordon.sdsc.edu

mosh --server=\$HOME/work/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server schnette@hopper.nersc.gov

mosh eschnetter@nvidia

NOmosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@shelob.hpc.lsu.edu

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@stampede.tacc.utexas.edu

mosh --server=\$HOME/trestles/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@trestles.sdsc.edu



rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@h2ologin.ncsa.illinois.edu:/mnt/a/u/sciteam/eschnett/src/cc/FunHPC/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@login-damiana.aei.mpg.de:datura/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit ux452368@gordon.sdsc.edu:gordon/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit schnette@hopper.nersc.gov:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@mike.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnetter@nvidia.pi.local:/xfs1/eschnetter/src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@shelob.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit eschnett@stampede.tacc.utexas.edu:/work/00507/eschnett/src/cc/mpi-rpc

rsync -Paz BUILD README Makefile .clang-format *.cc *.hh *.rst *.sh *.submit ux452368@trestles.sdsc.edu:trestles/src/mpi-rpc/



(source $HOME/SIMFACTORY/cereal-1.0.0/env.sh && source $HOME/SIMFACTORY/hwloc-1.10.0/env.sh && source $HOME/SIMFACTORY/jemalloc-3.6.0/env.sh && source $HOME/SIMFACTORY/llvm-3.5.0/env.sh && source $HOME/SIMFACTORY/openmpi-1.8.3/env.sh && source $HOME/SIMFACTORY/qthreads-1.10/env.sh && make -j8 wave)

(export SIMFACTORY_SIM=$HOME/Cbeta/simfactory3/sim && source $HOME/SIMFACTORY/cereal-1.0.0/env.sh && source $HOME/SIMFACTORY/hwloc-1.10.0/env.sh && source $HOME/SIMFACTORY/jemalloc-3.6.0/env.sh && source $HOME/SIMFACTORY/llvm-3.5.0/env.sh && source $HOME/SIMFACTORY/openmpi-1.8.3/env.sh && source $HOME/SIMFACTORY/qthreads-1.10/env.sh && make -j8 format && make -j8 wave)

(source $HOME/SIMFACTORY/cereal-1.1.0/env.sh && source $HOME/SIMFACTORY/hwloc-1.10.0/env.sh && source $HOME/SIMFACTORY/jemalloc-3.6.0/env.sh && source $HOME/SIMFACTORY/llvm-3.5.1/env.sh && source $HOME/SIMFACTORY/openmpi-1.8.4/env.sh && source $HOME/SIMFACTORY/qthreads-1.10/env.sh && make -j8 wave)



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



===============================================================================

10   wave-vector
20   wave
30   wave3d-grid
40   wave3d-tree
50   wave3d-grid 2d
60   wave3d-tree 2d
70   wave3d-grid 3d
80   wave3d-tree 3d

for r in 10; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave-vector& done; done; done
for r in 20; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave& done; done; done
for r in 30; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 40; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done
for r in 50; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 60; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done
for r in 70; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 80; do for n in 1 2 4 8; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done

for r in 30; do for n in 64 128 256 512 1024; do for t in 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 40; do for n in 64 128 256 512 1024; do for t in 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done

for r in 70; do for n in 16 32; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-grid& done; done; done
for r in 80; do for n in 16 32; do for t in 1 2 4 8 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done

for r in 81 82; do for n in 1 2 4 8 16 32; do for t in 16; do ./submit.stampede.sh $n 2 8 $[16*$n/$t] $t 1 $r ./wave3d-tree& done; done; done



for r in 10 20 30 40 50 60 70 80; do for t in 1 2 4 8 16; do for n in 1 2 4 8 16 32 64; do for f in job-n$n.s*.c*.p*.t$t.m*.r$r.log; do grep -H 'Time:' $f 2>/dev/null | tail -n +3 | head -n 1; done; done; done; done

n      s   c   p      t    m   r    time [sec]
==============================================
   1   2   8     16    1   1   40   0.37307
   2   2   8     32    1   1   40   0.427491
   4   2   8     64    1   1   40   0.465184
   8   2   8    128    1   1   40   0.531151
  16   2   8    256    1   1   40   0.645411
  32   2   8    512    1   1   40   0.712983
                       
   1   2   8      8    2   1   40   0.400241
   2   2   8     16    2   1   40   0.425358
   4   2   8     32    2   1   40   0.415151
   8   2   8     64    2   1   40   0.545102
  16   2   8    128    2   1   40   0.503033
  32   2   8    256    2   1   40   0.659562
                       
   1   2   8      4    4   1   40   0.592692
   2   2   8      8    4   1   40   0.611702
   4   2   8     16    4   1   40   0.594759
   8   2   8     32    4   1   40   0.789429
  16   2   8     64    4   1   40   0.690389
  32   2   8    128    4   1   40   0.692186
                       
   1   2   8      2    8   1   40   0.980978
   2   2   8      4    8   1   40   1.10006
   4   2   8      8    8   1   40   1.10913
   8   2   8     16    8   1   40   0.96546
  16   2   8     32    8   1   40   0.941995
  32   2   8     64    8   1   40   1.01773
                  
   1   2   8      1   16   1   40   0.2834
   2   2   8      2   16   1   40   0.583062
   4   2   8      4   16   1   40   0.589088
   8   2   8      8   16   1   40   0.673585
  16   2   8     16   16   1   40   0.645865
  32   2   8     32   16   1   40   0.699062
  64   2   8     64   16   1   40   0.924813
 128   2   8    128   16   1   40   0.991319
 256   2   8    256   16   1   40   1.37656
 512   2   8    512   16   1   40   1.76524
1024   2   8   1024   16   1   40   4.20874



n    s   c   p     t    m   r    time [sec]
===========================================
 1   2   8    16    1   1   80    1.682
 2   2   8    32    1   1   80    2.40876
 4   2   8    64    1   1   80    2.37647
 8   2   8   128    1   1   80    5.7332
16   2   8   256    1   1   80   10.6286
32   2   8   512    1   1   80   29.7104

 1   2   8     8    2   1   80    1.76972
 2   2   8    16    2   1   80    3.40507
 4   2   8    32    2   1   80    3.34446
 8   2   8    64    2   1   80    4.2547
16   2   8   128    2   1   80   22.1666
32   2   8   256    2   1   80   33.7306

 1   2   8     4    4   1   80    2.76874
 2   2   8     8    4   1   80    4.99106
 4   2   8    16    4   1   80    5.94931
 8   2   8    32    4   1   80   20.0566
16   2   8    64    4   1   80   17.1126
32   2   8   128    4   1   80   27.9274

 1   2   8     2    8   1   80    3.16762
 2   2   8     4    8   1   80    6.00488
 4   2   8     8    8   1   80    2.9103
 8   2   8    16    8   1   80   14.9037
16   2   8    32    8   1   80   28.6776
32   2   8    64    8   1   80   33.5375

 1   2   8     1   16   1   80    0.638679
 2   2   8     2   16   1   80    3.05386
 4   2   8     4   16   1   80    2.27791
 8   2   8     8   16   1   80   13.0968
16   2   8    16   16   1   80   22.9391
32   2   8    32   16   1   80   26.1299
