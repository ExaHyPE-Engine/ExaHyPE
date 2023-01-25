#!/bin/sh

#SBATCH -n 1
#SBATCH --array=1-6
#SBATCH -t 01:00:00
#SBATCH -J 100On
#SBATCH -o ./%A.%a.log
#SBATCH -e ./%A.%a.err
#SBATCH -p cosma7-rp
#SBATCH -A do009 
#SBATCH --exclusive

# prepare
module purge
module load oneAPI_comp/2021.1.0
module load likwid/5.2.0
module load python/3.10.1
module load intel_comp/2019
module load hdf5
module load gnu_comp/11.1.0

#python3 planet-orbit.py
export PATH="/cosma/home/dp004/dc-barr3/.local/bin:$PATH"
export PATH="/cosma/home/dp004/dc-barr3/software/bin:$PATH"
export JUPYTER_PATH=/cosma7/data/dp004/dc-barr3/Peano/python
export PYTHONPATH=/cosma7/data/dp004/dc-barr3/Peano/python  

# gcc build
export C_INCLUDE_PATH="/cosma/home/dp004/dc-barr3/software/include"
export CPLUS_INCLUDE_PATH="/cosma/home/dp004/dc-barr3/software/include"
export LIBRARY_PATH="/cosma/home/dp004/dc-barr3/software/lib:/cosma/home/dp004/dc-barr3/software/lib64"
export LD_LIBRARY_PATH="/cosma/home/dp004/dc-barr3/software/lib:/cosma/home/dp004/dc-barr3/software/lib64:$LD_LIBRARY_PATH"

# Peano 4 compilation with Clang
export OMPI_CC=/cosma/home/dp004/dc-barr3/software/bin/clang
export OMPI_CXX=/cosma/home/dp004/dc-barr3/software/bin/clang++

# set number of threads
export OMP_NUM_THREADS=28

# run the already compiled code

if [ $SLURM_ARRAY_TASK_ID -eq 1 ]
then
  likwid-perfctr -f -C 0 -g DATA           -o output_DATA.cvs           -O ./noh2D
elif [ $SLURM_ARRAY_TASK_ID -eq 2 ]
then
  likwid-perfctr -f -C 0 -g CACHES         -o output_CACHES.cvs         -O ./noh2D
elif [ $SLURM_ARRAY_TASK_ID -eq 3 ]
then
  likwid-perfctr -f -C 0 -g L2CACHE        -o output_L2CACHE.cvs        -O ./noh2D
elif [ $SLURM_ARRAY_TASK_ID -eq 4 ]
then
  likwid-perfctr -f -C 0 -g L3CACHE        -o output_L3CACHE.cvs        -O ./noh2D
elif [ $SLURM_ARRAY_TASK_ID -eq 5 ]
then
  likwid-perfctr -f -C 0 -g CYCLE_ACTIVITY -o output_CYCLE_ACTIVITY.cvs -O ./noh2D
elif [ $SLURM_ARRAY_TASK_ID -eq 6 ]
then
  likwid-perfctr -f -C 0 -g MEM            -o output_MEM.cvs            -O ./noh2D
fi


# report
echo ""
sacct -j $SLURM_JOBID --format=JobID,JobName,Partition,AllocCPUS,Elapsed,ExitCode
exit
