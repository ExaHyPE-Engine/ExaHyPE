#!/bin/sh

#SBATCH -n 1
#SBATCH -J clangOff
#SBATCH -o ./%j.log
#SBATCH -e ./%j.err
#SBATCH -t 00:30:00
#SBATCH -p multi
#SBATCH --qos=shortwide
#SBATCH --exclusive

# prepare
module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load likwid
module load python

# -----------------------------------------------------------------------------
# Peano 4 compilation with Clang
export JUPYTER_PATH=/nobackup/hgcq36/codes/Peano/python
export PYTHONPATH=/nobackup/hgcq36/codes/Peano/python

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
#export OMP_NUM_THREADS=24

# Run the code
#./run_1e-0.sh
#./run_1e-1.sh
./run_1e-2.sh
#./run_1e-3.sh

# report
echo ""
sacct -j $SLURM_JOBID --format=JobID,JobName,Partition,AllocCPUS,Elapsed,ExitCode
exit
