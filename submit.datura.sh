#! /bin/bash

set -e
set -u
#set -x

# User choices
if (($#<8)); then
    echo "Synopsis: $0 <nodes> <sockets/node> <cores/socket> <procs> <threads/proc> <smts/thread> <run_id> <program> <arguments...>" >&2
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
shift 7
prog="$*"
echo "   nodes=$nodes sockets/node=$sockets_per_node cores/socket=$cores_per_socket proc=$procs threads/proc=$threads_per_proc smts/thread=$smts_per_thread"
echo "   program: $prog"

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
    ppr="$procs_per_core:core"
    bind_to='core'
elif (($procs_per_socket)); then
    ppr="$procs_per_socket:socket"
    bind_to='socket'
elif (($procs_per_node)); then
    ppr="$procs_per_node:node"
    bind_to='none'
fi

cat >$HOME/datura/src/mpi-rpc/job.$id.sub <<EOF
#! /bin/bash

#$ -V
#$ -q daturamon.q
#$ -r n
#$ -l h_rt=0:10:00
#$ -pe daturamon $cores
#$ -N job.$id
#$ -m abe
#$ -o $HOME/datura/src/mpi-rpc/job.$id.out
#$ -e $HOME/datura/src/mpi-rpc/job.$id.err

# nodes:   $nodes
# sockets: $sockets   sockets/node: $[$sockets/$nodes]
# cores:   $cores   cores/socket: $[$cores/$sockets]

# procs:   $procs
# threads: $threads   threads/proc: $[$threads/$procs]
# smts:    $smts   smts/thread: $[$smts/$threads]

# run: $run

set -e
set -u
set -x
cd $HOME/datura/src/mpi-rpc
export SIMFACTORY_SIM=$HOME/datura/work/Cbeta/simfactory3/sim
source $HOME/datura/SIMFACTORY/all-all/env.sh

echo '[BEGIN ENV]'
env | sort
echo '[END ENV]'

echo '[BEGIN NODES]'
cat \$PE_HOSTFILE
echo '[END NODES]'

date
echo '[BEGIN MPIRUN]'
# --mca btl self,sm,openib
# --mca btl self,sm,tcp
\$MPIRUN                                                                \\
    -np $procs                                                          \\
    --map-by ppr:$ppr                                                   \\
    --display-map                                                       \\
    --mca btl self,sm,tcp                                               \\
    --bind-to $bind_to                                                  \\
    --report-bindings                                                   \\
    -x RPC_NODES=$nodes                                                 \\
    -x RPC_CORES=$cores_per_node                                        \\
    -x RPC_PROCESSES=$procs                                             \\
    -x RPC_THREADS=$threads_per_proc                                    \\
    -x QTHREAD_NUM_SHEPHERDS=$proc_sockets                              \\
    -x QTHREAD_NUM_WORKERS_PER_SHEPHERD=$threads_per_proc_socket        \\
    -x QTHREAD_STACK_SIZE=524288                                        \\
    -x QTHREAD_GUARD_PAGES=1                                            \\
    -x QTHREAD_INFO=1                                                   \\
    $prog                                                               \\
    --hpx:ini=hpx.parcel.mpi.enable=0                                   \\
    --hpx:numa-sensitive                                                \\
    --hpx:threads=$threads_per_proc                                     \\
    >job.$id.log 2>&1
echo '[END MPIRUN]'
date
EOF

: >$HOME/datura/src/mpi-rpc/job.$id.out
: >$HOME/datura/src/mpi-rpc/job.$id.err
: >$HOME/datura/src/mpi-rpc/job.$id.log
qsub $HOME/datura/src/mpi-rpc/job.$id.sub
