#!/bin/sh

#SBATCH -n 1
#SBATCH --array=1-4
#SBATCH -t 12:00:00
#SBATCH -o ./%A.%a.log
#SBATCH -e ./%A.%a.err
#SBATCH -p multi
#SBATCH --exclusive

# prepare
module purge
module load oneapi
export FLAVOUR_NOCONFLICT=1
module load gcc
module load likwid
module load python

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
export OMP_NUM_THREADS=$1

MAX_CORE_ID=$(($1-1))
echo "MAX_CORE_ID="$MAX_CORE_ID 

if [ $SLURM_ARRAY_TASK_ID -eq 1 ]
then
  likwid-perfctr -f -C 0-$MAX_CORE_ID -g DATA   -o output_DATA_cores_$1_on.cvs   -O ./noh2D > p4_output_DATA_cores_$1_on.txt
elif [ $SLURM_ARRAY_TASK_ID -eq 2 ]
then
  likwid-perfctr -f -C 0-$MAX_CORE_ID -g CACHE  -o output_CACHE_cores_$1_on.cvs  -O ./noh2D > p4_output_CACHE_cores_$1_on.txt
elif [ $SLURM_ARRAY_TASK_ID -eq 3 ]
then
  likwid-perfctr -f -C 0-$MAX_CORE_ID -g CPI    -o output_CPI_cores_$1_on.cvs    -O ./noh2D > p4_output_CPI_cores_$1_on.txt
elif [ $SLURM_ARRAY_TASK_ID -eq 4 ]
then
  likwid-perfctr -f -C 0-$MAX_CORE_ID -g MEM    -o output_MEM_cores_$1_on.cvs    -O ./noh2D > p4_output_MEM_cores_$1_on.txt
fi


# report
echo ""
sacct -j $SLURM_JOBID --format=JobID,JobName,Partition,AllocCPUS,Elapsed,ExitCode
exit
