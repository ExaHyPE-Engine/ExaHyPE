#!/bin/bash
#SBATCH -J MemLayout
#SBATCH --account=pn52yu
#SBATCH --time=12:00:00
#SBATCH --export=NONE
#SBATCH --partition=general
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1  #one MPI job per GPU tile
#SBATCH --array 1,2,4,7,8,14,28,56,128

module load intel gcc intel-mpi

export OMP_NUM_THREADS=$SLURM_ARRAY_TASK_ID

export 

./run-memory-layout.sh 

tar -czvf SuperMUC-$OMP_NUM_THREADS-threads.tar.gz *$OMP_NUM_THREADS-threads.out

