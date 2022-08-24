#!/bin/bash
#SBATCH --job-name=single-node
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH -c 128
#SBATCH --time=08:30:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --array 1,2,3,4,6,8,10,12,14,16,20,24,28,32,36,40,44,48,52,56,64,72,80,88,96,104,112,120,128

source /etc/profile.d/modules.sh

module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
#module load intelmpi


export OMP_PROC_BIND=close
#$SLURM_JOB_NUM_NODES

for extension in "0"
do
 for executable in benchmark-FV-4 benchmark-FV-6 benchmark-FV-8 benchmark-RK2DG2 benchmark-RK3DG3 benchmark-RK4DG4
 do 
  ./$executable-no-opt-$extension --threads $SLURM_ARRAY_TASK_ID > single-node-$executable-no-opt-$extension-$SLURM_ARRAY_TASK_ID-cores.data
  ./$executable-opt-$extension    --threads $SLURM_ARRAY_TASK_ID > single-node-$executable-opt-$extension-$SLURM_ARRAY_TASK_ID-cores.data
 done
done
 


