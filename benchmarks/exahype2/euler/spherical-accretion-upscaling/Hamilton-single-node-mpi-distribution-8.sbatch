#!/bin/bash
#SBATCH --job-name=mpi-dist-8
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=16
#SBATCH --time=00:30:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --qos=shortwide

source /etc/profile.d/modules.sh

module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load intelmpi


export OMP_PROC_BIND=close
#$SLURM_JOB_NUM_NODES

numactl --show

for type in reg-0
do
 for solver in FV-8 RK4DG4
 do 
  executable=benchmark-$type-opt-$solver
  echo $executable
  mpirun ./$executable > single-node-$executable-$SLURM_NTASKS_PER_NODE-mpi-tasks-on-node.data
 done
done
 


