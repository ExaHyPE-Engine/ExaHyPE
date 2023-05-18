#!/bin/bash

# Request resources:
#SBATCH -c 2           # 40 CPU core
#SBATCH --mem=100G       # memory required, up to 250G on standard nodes.
#SBATCH --gres=tmp:50G  # temporary disk space required on the compute node ($TMPDIR), up to 400G
#SBATCH --time=0:25:0   # time limit for job (format:  days-hours:minutes:seconds)
#SBATCH -o standard_output_file.%J.out
#SBATCH -e standard_error_file.%J.err

# Run in the 'shared' queue (job may share node with other jobs)
#SBATCH -p shared

export PYTHONPATH=$PYTHONPATH:$NOBACKUP/repos/Peano/python:/apps/developers/compilers/python/3.9.9/1/default/lib/python3.9/site-packages

# Commands to be run:
module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load intelmpi #intelmpi
export I_MPI_CC=icx
export I_MPI_CXX=icpx 
module load python/3.9.9
module load paraview

# actual script
echo $PYTHONPATH
echo
./peano4petsc

#visualise
pvpython ../../../../python/peano4/visualisation/render.py --filter-fine-grid solution-Poisson.peano-patch-file
pvpython ../../../../python/peano4/visualisation/render.py --filter-fine-grid rhs-Poisson.peano-patch-file
