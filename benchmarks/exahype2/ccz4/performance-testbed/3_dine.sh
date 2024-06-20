#!/bin/bash -l

#SBATCH -o %A.out
#SBATCH -e %A.err
#SBATCH -p bluefield1
#SBATCH -A durham
#SBATCH -t 1:00:00
#SBATCH --ntasks-per-node=1
#SBATCH --nodes=1
#SBATCH -J ccz4

echo "My SLURM_ARRAY_TASK_ID: " $SLURM_ARRAY_TASK_ID

module purge
module load intel_comp/2022.3.0 compiler mpi 
module load gnu_comp/11.1.0
export FLAVOUR_NOCONFLICT=1
module load gsl

export UCX_TLS=ud,sm,self

export OMP_NUM_THREADS=16

mpirun ./peano4_test --timeout 3600


