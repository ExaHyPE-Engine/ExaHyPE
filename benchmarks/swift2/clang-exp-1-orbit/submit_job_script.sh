#!/bin/sh

#SBATCH -n 1
#SBATCH -t 00:30:00
#SBATCH -J clangOn
#SBATCH -o ./logs/%j.log
#SBATCH -e ./logs/%j.err
#SBATCH -p cosma7-rp
#SBATCH -A do009 
#SBATCH --exclusive

# prepare
module purge
module load oneAPI_comp/2021.1.0
module load likwid/5.2.0
module load python/3.10.1

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

# Tell OpenMP to use the job's CPU cores
#export OMP_NUM_THREADS=24
export OMP_NUM_THREADS=1

# Run the code
./run_1e-0.sh
#./run_1e-1.sh
#./run_1e-2.sh
#./run_1e-3.sh

# report
echo ""
sacct -j $SLURM_JOBID --format=JobID,JobName,Partition,AllocCPUS,Elapsed,ExitCode
exit
