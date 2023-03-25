#!/bin/sh

#SBATCH -n 1
#SBATCH -t 24:00:00
#SBATCH -J OnM10
#SBATCH -o ./%j.log
#SBATCH -e ./%j.err
#SBATCH -p multi
#SBATCH --exclusive

# prepare
module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load likwid
module load python
#module load intel_comp/2019


# -----------------------------------------------------------------------------
# Peano 4 compilation with Clang
export JUPYTER_PATH=/nobackup/hgcq36/codes/Peano/python
export PYTHONPATH=/nobackup/hgcq36/codes/Peano/python

# libomp.so path (for clang compression in Peano)
#export LD_LIBRARY_PATH=/nobackup/hgcq36/libs:$LD_LIBRARY_PATH

# own gcc compilation 
export PATH="/home/hgcq36/software/bin:$PATH"
export C_INCLUDE_PATH="/home/hgcq36/software/include"
export CPLUS_INCLUDE_PATH="/home/hgcq36/software/include"
export LIBRARY_PATH="/home/hgcq36/software/lib:/home/hgcq36/software/lib64"
export LD_LIBRARY_PATH="/home/hgcq36/software/lib:/home/hgcq36/software/lib64:$LD_LIBRARY_PATH"

# own Clang compiler
export OMPI_CC=/nobackup/hgcq36/llvm-13-release/bin/clang
export OMPI_CXX=/nobackup/hgcq36/llvm-13-release/bin/clang++



# Run the code
./run.sh


# report
echo ""
sacct -j $SLURM_JOBID --format=JobID,JobName,Partition,AllocCPUS,Elapsed,ExitCode
exit
