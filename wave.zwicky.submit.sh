#! /bin/bash

set -e
set -u
#set -x

# User choices
if (($#!=7)); then
    echo "Synopsis: $0 <nodes> <sockets/node> <cores/socket> <procs> <threads/proc> <smts/thread> <run_id>" >&2
    exit 1
fi
echo "User choices:"
nodes=$1
sockets_per_node=$2
cores_per_socket=$3
procs=$4
threads_per_proc=$5
smts_per_thread=$6
run=$7
echo "   nodes=$nodes sockets/node=$sockets_per_node cores/socket=$cores_per_socket proc=$procs threads/proc=$threads_per_proc smts/thread=$smts_per_thread"

# Automatically calculated quantities
echo "Automatically calculated quantities:"
sockets=$[$sockets_per_node*$nodes]
cores=$[$cores_per_socket*$sockets]
echo "   nodes=$nodes sockets=$sockets cores=$cores"
threads=$[$threads_per_proc*$procs]
smts=$[$smts_per_thread*$threads]
echo "   procs=$procs threads=$threads smts=$smts"

id="n$nodes.s$sockets_per_node.c$cores_per_socket.p$procs.t$threads_per_proc.m$smts_per_thread.r$run"

# Settings specific to the submit script
cores_per_node=$[$cores_per_socket*$sockets_per_node]
procs_per_core=$[$procs/$cores]
procs_per_socket=$[$procs/$sockets]
procs_per_node=$[$procs/$nodes]
sockets_per_proc=$[$sockets/$procs]
sockets_per_proc_ceil=$[$sockets_per_proc ? $sockets_per_proc : 1]

proc_sockets=$[$sockets_per_proc_ceil <= $threads_per_proc ? $sockets_per_proc_ceil : $threads_per_proc]
threads_per_proc_socket=$[$threads_per_proc/$proc_sockets]

if (($procs_per_core)); then
    ppr=''
    bind_to='--bind-to-core'
elif (($procs_per_socket)); then
    ppr="--npersocket $procs_per_socket"
    bind_to='--bind-to-socket'
elif (($procs_per_node)); then
    ppr="--npernode $procs_per_node"
    bind_to='--bind-to-none'
fi

cat >$HOME/src/cc/FunHPC.cxx/wave.$id.sub <<EOF
#! /bin/bash

#PBS -V
#PBS -A sxs
#PBS -q productionQ
#PBS -r n
#PBS -l walltime=0:10:00
#PBS -l nodes=$nodes:ppn=$cores_per_node
#PBS -N wave.$id
#PBS -m abe
#PBS -o $HOME/src/cc/FunHPC.cxx/wave.$id.out
#PBS -e $HOME/src/cc/FunHPC.cxx/wave.$id.err

# nodes:   $nodes
# sockets: $sockets   sockets/node: $[$sockets/$nodes]
# cores:   $cores   cores/socket: $[$cores/$sockets]

# procs:   $procs
# threads: $threads   threads/proc: $[$threads/$procs]
# smts:    $smts   smts/thread: $[$smts/$threads]

set -e
set -u
set -x
cd $HOME/src/cc/FunHPC.cxx
source ~jlippuner/.module_funhpc

echo '[BEGIN ENV]'
env | sort
echo '[END ENV]'

echo '[BEGIN NODES]'
cat \$PBS_NODEFILE
echo '[END NODES]'

date
echo '[BEGIN MPIRUN]'
mpirun                                                                  \\
    -np $procs                                                          \\
    $ppr                                                                \\
    --display-map                                                       \\
    --mca btl self,sm,openib                                            \\
    $bind_to                                                            \\
    --report-bindings                                                   \\
    -x RPC_NODES=$nodes                                                 \\
    -x RPC_CORES=$cores_per_node                                        \\
    -x RPC_PROCESSES=$procs                                             \\
    -x RPC_THREADS=$threads_per_proc                                    \\
    -x QTHREAD_NUM_SHEPHERDS=$proc_sockets                              \\
    -x QTHREAD_NUM_WORKERS_PER_SHEPHERD=$threads_per_proc_socket        \\
    -x QTHREAD_STACK_SIZE=65536                                         \\
    -x QTHREAD_INFO=0                                                   \\
    ./wave                                                              \\
    --hpx:ini=hpx.parcel.mpi.enable=0                                   \\
    --hpx:numa-sensitive                                                \\
    --hpx:threads=$threads_per_proc                                     \\
    >wave.$id.log 2>&1
echo '[END MPIRUN]'
date
EOF

: >$HOME/src/cc/FunHPC.cxx/wave.$id.out
: >$HOME/src/cc/FunHPC.cxx/wave.$id.err
: >$HOME/src/cc/FunHPC.cxx/wave.$id.log
qsub $HOME/src/cc/FunHPC.cxx/wave.$id.sub
