#!/bin/bash
#SBATCH --job-name="noh32"
#SBATCH -o ./logs/%A.out
#SBATCH -e ./logs/%A.err
#SBATCH -p multi
#SBATCH --exclusive
#SBATCH --qos=shortwide
#SBATCH -t 00:29:00
#SBATCH --nodes=1
#SBATCH --mail-type=END
#SBATCH --mail-user=cristian.g.barrera@durham.ac.uk

# Load modules
module purge
#module load oneapi
export FLAVOUR_NOCONFLICT=1
#module load gcc/11.2
module load likwid   
module load python/3.9.9

# Peano 4 compilation with Clang
export JUPYTER_PATH=/nobackup/hgcq36/codes/Peano/python
export PYTHONPATH=/nobackup/hgcq36/codes/Peano/python:$PYTHONPATH

# own gcc compilation 
export PATH="/home/hgcq36/software/bin:$PATH"
export C_INCLUDE_PATH="/home/hgcq36/software/include"
export CPLUS_INCLUDE_PATH="/home/hgcq36/software/include"
export LIBRARY_PATH="/home/hgcq36/software/lib:/home/hgcq36/software/lib64"
export LD_LIBRARY_PATH="/home/hgcq36/software/lib:/home/hgcq36/software/lib64:$LD_LIBRARY_PATH"

# own Clang compiler
export OMPI_CC=/nobackup/hgcq36/llvm-13-release/bin/clang
export OMPI_CXX=/nobackup/hgcq36/llvm-13-release/bin/clang++

# Tell OpenMP to use the job's CPU cores
export OMP_NUM_THREADS=1

# Build executables 
python3 minimal-sph-peano-noh-problem.py -m release -dist 0.03125 -p     # 32 particles
#python3 minimal-sph-peano-noh-problem.py -m release -dist 0.015625 -p   # 64 particles
#python3 minimal-sph-peano-noh-problem.py -m release -dist 0.0078125     # 128 particles
#python3 minimal-sph-peano-noh-problem.py -m release -dist 0.00390625    # 256 particles

make clean && make -j128

# Run the code
#To measure the behaviour, we use the following launch commands (within the Slurm scripts):
likwid-perfctr -C 0 -g MEM   -o output_MEM.cvs   -O ./peano4
#likwid-perfctr -C 0 -g CPI   -o output_CPI.cvs   -O ./peano4
#likwid-perfctr -C 0 -g CACHE -o output_CACHE.cvs -O ./peano4
#likwid-perfctr -C 0 -g DATA  -o output_DATA.cvs  -O ./peano4

