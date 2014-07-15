mosh eschnetter@nvidia

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@shelob.hpc.lsu.edu

mosh --server=\$HOME/SIMFACTORY/mosh-1.2.4/install/bin/mosh-server eschnett@stampede.tacc.utexas.edu



rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnetter@nvidia.pi.local:/xfs1/eschnetter/compute/src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnett@shelob.hpc.lsu.edu:src/mpi-rpc/

rsync -Paz BUILD README Makefile *.cc *.hh *.rst *.sh *.submit eschnett@stampede.tacc.utexas.edu:src/mpi-rpc/



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
