#!/bin/bash
#SBATCH -t 10:00:00
#SBATCH --exclusive
#SBATCH -p par7.q
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH -o swe_run.%A.out
#SBATCH -e swe_run.%A.err
#SBATCH --mail-type=END
#SBATCH --mail-user=marion.weinzierl@durham.ac.uk

export CLAW=/ddn/data/cvxs42/clawpack_src/clawpack-v5-8-0
export PYTHONPATH=/ddn/data/cvxs42/Peano/python

module purge
module load gsl/intel/2.4
module load intel/2020.4 gcc/9.3.0 intelmpi/intel/2019.6 python/3.6.8
module unload gcc/8.2.0

#mpirun -cpus-per-rank 24 ./peano4 --threads $SLURM_CPUS_ON_NODE

mpirun ./peano4 --threads $SLURM_CPUS_ON_NODE 
