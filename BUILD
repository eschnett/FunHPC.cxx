mosh --server=\$HOME/gordon/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@gordon.sdsc.edu

mosh eschnetter@nvidia

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@shelob.hpc.lsu.edu

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@stampede.tacc.utexas.edu

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server ux452368@trestles.sdsc.edu



rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit ux452368@gordon.sdsc.edu:gordon/src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnetter@nvidia.pi.local:/xfs1/eschnetter/compute/src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnett@shelob.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnett@stampede.tacc.utexas.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit ux452368@trestles.sdsc.edu:src/mpi-rpc/



for n in 1 2 4 8 16 32; do
    for s in 2; do
        for c in 8; do
            for t in 1 2 4 8 16; do
                for m in 1; do
                    p=$[$n*$s*$c/($t*$m)]
                    ./wave.shelob.submit.sh $n $s $c $p $t $m
                done
            done
        done
    done
done

(. ~/SIMFACTORY/all-all/env.sh && make -j8 wave) && for n in 4 8 16 32; do for t in 1 16; do ./wave.stampede.submit.sh $n 2 8 $[$n*16/$t] $t 1& done; done; wait

for f in wave.n*.s*.c*.p*.t*.m*.log; do grep -H 'Time:' $f | tail -n +2; done 



GOOD:
- bethe
- gordon
- nvidia
- redshift
- shelob
- titan
- trestles

PROBLEM:
- lonestar (c99)
- stampede (openmpi segfault)
- zwicky (c99)
