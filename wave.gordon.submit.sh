#! /bin/bash

set -e
set -u
#set -x

# User choices
if (($#!=6)); then
    echo "Synopsis: $0 <nodes> <sockets/node> <cores/socket> <procs> <threads/proc> <smts/thread>" >&2
    exit 1
fi
echo "User choices:"
nodes=$1
sockets_per_node=$2
cores_per_socket=$3
procs=$4
threads_per_proc=$5
smts_per_thread=$6
echo "   nodes=$nodes sockets/node=$sockets_per_node cores/socket=$cores_per_socket proc=$procs threads/proc=$threads_per_proc smts/thread=$smts_per_thread"

# Automatically calculated quantities
echo "Automatically calculated quantities:"
sockets=$[$sockets_per_node*$nodes]
cores=$[$cores_per_socket*$sockets]
echo "   nodes=$nodes sockets=$sockets cores=$cores"
threads=$[$threads_per_proc*$procs]
smts=$[$smts_per_thread*$threads]
echo "   procs=$procs threads=$threads smts=$smts"

id="n$nodes.s$sockets_per_node.c$cores_per_socket.p$procs.t$threads_per_proc.m$smts_per_thread"

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
    ppr="$procs_per_core:core"
    bind_to='core'
elif (($procs_per_socket)); then
    ppr="$procs_per_socket:socket"
    bind_to='socket'
elif (($procs_per_node)); then
    ppr="$procs_per_node:node"
    bind_to='none'
fi

cat >$HOME/gordon/src/mpi-rpc/wave.$id.sub <<EOF
#! /bin/bash

#PBS -A TG-PHY060027N
#PBS -q normal
#PBS -r n
#PBS -l walltime=0:10:00
#PBS -l nodes=$nodes:ppn=$cores_per_node
#PBS -V
#PBS -N wave.$id
#PBS -m abe
#PBS -o $HOME/gordon/src/mpi-rpc/wave.$id.out
#PBS -e $HOME/gordon/src/mpi-rpc/wave.$id.err

# nodes:   $nodes
# sockets: $sockets   sockets/node: $[$sockets/$nodes]
# cores:   $cores   cores/socket: $[$cores/$sockets]

# procs:   $procs
# threads: $threads   threads/proc: $[$threads/$procs]
# smts:    $smts   smts/thread: $[$smts/$threads]

set -e
set -u
set -x
cd $HOME/gordon/src/mpi-rpc
source $HOME/gordon/SIMFACTORY/all-all/env.sh

echo '[BEGIN ENV]'
env | sort
echo '[END ENV]'

echo '[BEGIN NODES]'
cat \$PBS_NODEFILE
echo '[END NODES]'

date
echo '[BEGIN MPIRUN]'
\$MPIRUN                                                                \\
    -np $procs                                                          \\
    --map-by ppr:$ppr                                                   \\
    --display-map                                                       \\
    --mca btl self,sm,openib                                            \\
    --bind-to $bind_to                                                  \\
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

: >$HOME/gordon/src/mpi-rpc/wave.$id.out
: >$HOME/gordon/src/mpi-rpc/wave.$id.err
: >$HOME/gordon/src/mpi-rpc/wave.$id.log
qsub $HOME/gordon/src/mpi-rpc/wave.$id.sub
