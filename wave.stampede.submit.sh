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
    ppr="$procs_per_core:core"
    bind_to='core'
elif (($procs_per_socket)); then
    ppr="$procs_per_socket:socket"
    bind_to='socket'
elif (($procs_per_node)); then
    ppr="$procs_per_node:node"
    bind_to='none'
fi

# Specific to Stampede
if (($nodes <= 256)); then
    partition='normal'
else
    partition='large'
fi

cat >$HOME/src/mpi-rpc/wave.$id.sub <<EOF
#! /bin/bash

#SBATCH --verbose
#SBATCH --account=TG-PHY100033
#SBATCH --partition=$partition
#SBATCH --time=0:10:00
#SBATCH --nodes=$nodes --ntasks=$procs
#SBATCH --job-name=wave.$id
#SBATCH --mail-type=ALL
#SBATCH --mail-user=schnetter@gmail.com
#SBATCH --output=$HOME/src/mpi-rpc/wave.$id.out
#SBATCH --error=$HOME/src/mpi-rpc/wave.$id.err

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
cd $HOME/src/mpi-rpc
export SIMFACTORY_SIM=$HOME/work/Cbeta/simfactory3/sim
source $HOME/SIMFACTORY/all-all/env.sh

echo '[BEGIN ENV]'
env | sort
echo '[END ENV]'

echo '[BEGIN NODES]'
echo \$SLURM_NODELIST
hostfile=/tmp/hostfile.\$SLURM_JOB_ID
scontrol show hostnames \$SLURM_NODELIST | tee \$hostfile
echo '[END NODES]'

date
echo '[BEGIN MPIRUN]'
unset SLURM_CHECKPOINT_IMAGE_DIR
unset SLURM_CPUS_ON_NODE
unset SLURMD_NODENAME
unset SLURM_GTIDS
unset SLURM_JOB_CPUS_PER_NODE
unset SLURM_JOB_ID
unset SLURM_JOBID
unset SLURM_JOB_NAME
unset SLURM_JOB_NODELIST
unset SLURM_JOB_NUM_NODES
unset SLURM_LOCALID
unset SLURM_NNODES
unset SLURM_NODE_ALIASES
unset SLURM_NODEID
unset SLURM_NODELIST
unset SLURM_NPROCS
unset SLURM_NTASKS
unset SLURM_PRIO_PROCESS
unset SLURM_PROCID
unset SLURM_QUEUE
unset SLURM_SUBMIT_DIR
unset SLURM_SUBMIT_HOST
unset SLURM_TACC_ACCOUNT
unset SLURM_TACC_CORES
unset SLURM_TACC_JOBNAME
unset SLURM_TACC_NCORES_SET
unset SLURM_TACC_NNODES_SET
unset SLURM_TACC_NODES
unset SLURM_TACC_RUNLIMIT_MINS
unset SLURM_TASK_PID
unset SLURM_TASKS_PER_NODE
unset SLURM_TOPOLOGY_ADDR
unset SLURM_TOPOLOGY_ADDR_PATTERN
\$MPIRUN                                                                \\
    --verbose                                                           \\
    --hostfile \$hostfile                                               \\
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
    -x QTHREAD_STACK_SIZE=500000                                        \\
    -x QTHREAD_INFO=0                                                   \\
    ./wave                                                              \\
    --hpx:ini=hpx.parcel.mpi.enable=0                                   \\
    --hpx:numa-sensitive                                                \\
    --hpx:threads=$threads_per_proc                                     \\
    >wave.$id.log 2>&1
echo '[END MPIRUN]'
date

rm -f \$hostfile
EOF

: >$HOME/src/mpi-rpc/wave.$id.out
: >$HOME/src/mpi-rpc/wave.$id.err
: >$HOME/src/mpi-rpc/wave.$id.log
sbatch $HOME/src/mpi-rpc/wave.$id.sub
