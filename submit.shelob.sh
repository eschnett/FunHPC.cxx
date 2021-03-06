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
sockets=$((sockets_per_node*nodes))
cores=$((cores_per_socket*sockets))
echo "   nodes=$nodes sockets=$sockets cores=$cores"
threads=$((threads_per_proc*procs))
smts=$((smts_per_thread*threads))
echo "   procs=$procs threads=$threads smts=$smts"

id="n$nodes.s$sockets_per_node.c$cores_per_socket.p$procs.t$threads_per_proc.m$smts_per_thread.r$run"

# Settings specific to the submit script
cores_per_node=$((cores_per_socket*sockets_per_node))
procs_per_core=$((procs/cores))
procs_per_socket=$((procs/sockets))
procs_per_node=$((procs/nodes))
sockets_per_proc=$((sockets/procs))
sockets_per_proc_ceil=$((sockets_per_proc ? sockets_per_proc : 1))

proc_sockets=$((sockets_per_proc_ceil <= threads_per_proc ? sockets_per_proc_ceil : threads_per_proc))
threads_per_proc_socket=$((threads_per_proc/proc_sockets))

if ((procs_per_core)); then
    ppr="$procs_per_core:core"
    bind_to='core'
elif ((procs_per_socket)); then
    ppr="$procs_per_socket:socket"
    bind_to='socket'
elif ((procs_per_node)); then
    ppr="$procs_per_node:node"
    bind_to='none'
fi

cat >job-$id.sub <<EOF
#! /bin/bash

#PBS -V
#PBS -A hpc_numrel06
#PBS -q checkpt
#PBS -r n
#PBS -l walltime=2:00:00
#PBS -l nodes=$nodes:ppn=$cores_per_node
#PBS -N job-$id
#PBS -m abe
#PBS -o $(pwd)/job-$id.out
#PBS -e $(pwd)/job-$id.err

# nodes:   $nodes
# sockets: $sockets   sockets/node: $((sockets/nodes))
# cores:   $cores   cores/socket: $((cores/sockets))

# procs:   $procs
# threads: $threads   threads/proc: $((threads/procs))
# smts:    $smts   smts/thread: $((smts/threads))

# run: $run

set -e
set -u
set -x
cd $(pwd)

pushd /project/eschnett/shelob/software && source cereal-1.1.2/env.sh && source hwloc-1.11.0/env.sh && source jemalloc-3.6.0/env.sh && source llvm-3.6.2/env.sh && source openmpi-v2.x-dev-100-g26c3f03/env.sh && source qthreads-1.10/env.sh && popd

echo '[BEGIN ENV]'
env | sort
echo '[END ENV]'

echo '[BEGIN NODES]'
cat \$PBS_NODEFILE
echo '[END NODES]'

date
echo '[BEGIN MPIRUN]'
# --mca btl self,sm,openib
# --mca btl self,sm,tcp
\$MPIRUN                                                                \\
    -np $procs                                                          \\
    --map-by ppr:$ppr                                                   \\
    --display-map                                                       \\
    --mca btl self,sm,openib                                            \\
    --bind-to $bind_to                                                  \\
    --report-bindings                                                   \\
    -x QTHREAD_NUM_SHEPHERDS=$proc_sockets                              \\
    -x QTHREAD_NUM_WORKERS_PER_SHEPHERD=$threads_per_proc_socket        \\
    -x QTHREAD_STACK_SIZE=65536                                         \\
    -x QTHREAD_GUARD_PAGES=0                                            \\
    -x QTHREAD_INFO=0                                                   \\
    $prog                                                               \\
    --hpx:ini hpx.stacks.use_guard_pages=0                              \\
    --hpx:numa-sensitive                                                \\
    --hpx:threads $threads_per_proc                                     \\
    >job-$id.log 2>&1
echo '[END MPIRUN]'
date
EOF

: >job-$id.out
: >job-$id.err
: >job-$id.log
qsub job-$id.sub
