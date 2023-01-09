#!/bin/bash
#SBATCH --job-name=single-node-FV
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH -c 128
#SBATCH --time=24:00:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --array 1,2,3,4,6,8,10,12,14,16,20,24,28,32,36,40,44,48,52,56,64,72,80,88,96,104,112,120,128

source /etc/profile.d/modules.sh

module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load intelmpi


export OMP_PROC_BIND=close
#$SLURM_JOB_NUM_NODES

for type in reg-0
do
 for solver in FV-4 FV-6 FV-8
 do 
  executable=benchmark-$type-no-opt-$solver
  echo $executable
  mpirun ./$executable --threads $SLURM_ARRAY_TASK_ID > single-node-$executable-$SLURM_ARRAY_TASK_ID-cores.data
  executable=benchmark-$type-opt-$solver
  echo $executable
  mpirun ./$executable --threads $SLURM_ARRAY_TASK_ID > single-node-$executable-$SLURM_ARRAY_TASK_ID-cores.data
 done
done
 


