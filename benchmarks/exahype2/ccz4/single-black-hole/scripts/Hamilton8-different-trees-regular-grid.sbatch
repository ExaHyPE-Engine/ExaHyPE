#!/bin/bash
#SBATCH --job-name=trees-regular
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH -n 1
#SBATCH -c 128
#SBATCH --time=12:00:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
#SBATCH --array=1,2,3,4,6,8,9,16,27,32,64,128,256 
 
source /etc/profile.d/modules.sh
 
module purge
module load oneapi/2025.2
export FLAVOUR_NOCONFLICT=1
module load gcc/15.2
export I_MPI_CXX=icpx
 
./peano-regular-$SLURM_ARRAY_TASK_ID-trees  > results-regular-$SLURM_ARRAY_TASK_ID.out

