#!/bin/bash

# Request resources:
#SBATCH -p shared
#SBATCH -N 1
#SBATCH -n 3
#SBATCH -c 2
#SBATCH --time=1:00:00   # time limit for job (format:  days-hours:minutes:seconds)
#SBATCH -o %J.out
#SBATCH -e %J.err
#SBATCH -J Hardcode-demo

export OMP_NUM_THREADS=2

source $HOME/peano-python-api/bin/activate

module load gcc openblas/0.3.18omp
export FLAVOUR_NOCONFLICT=1
module load oneapi/2025.2
module load gcc intelmpi #intel openmpi
#export I_MPI_CC=icx
export I_MPI_CXX=icpx
module load python/3.13.9

export PYTHONPATH=/nobackup/lcgk69/Peano/src:$PYTHONPATH

mpirun -np 3 bash -c 'rank=${PMI_RANK:-$OMPI_COMM_WORLD_RANK}; exec >"stdout-$rank.log" 2>"stderr-$rank.log"; ./HardcodedLB.demo'
#mpirun -np 3 ./HardcodedLB.demo
