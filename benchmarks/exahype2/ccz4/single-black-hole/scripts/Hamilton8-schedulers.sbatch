#!/bin/bash
#SBATCH --job-name=trees-scheduler
#SBATCH -p multi
#SBATCH --nodes=1
#SBATCH -n 1
#SBATCH -c 128
#SBATCH --time=12:00:00
#SBATCH --mail-type=END
#SBATCH --mail-user=tobias.weinzierl@durham.ac.uk
 
source /etc/profile.d/modules.sh
 
module purge
module load oneapi/2025.2
export FLAVOUR_NOCONFLICT=1
module load gcc/15.2
export I_MPI_CXX=icpx

module load tbb/2022.2

 

for SCHEDULER in no-tasks native gpu-lazy-evaluation gpu-timed-execution gpu-eager-consumer-tasks
do
  ./peano-regular-$SCHEDULER  > results-scheduler-regular-$SCHEDULER.out
  ./peano-adaptive-$SCHEDULER > results-scheduler-adaptive-$SCHEDULER.out
done
