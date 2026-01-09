#!/bin/bash -l
#SBATCH -o slurm-0.5.out
#SBATCH -e slurm-0.5.err
#SBATCH -p bluefield1
#SBATCH -A durham
#SBATCH -t 72:00:00
#SBATCH --nodes=1


module purge

module load oneAPI
module load intel_mpi
module load gnu_comp/11.1.0
export I_MPI_CXX=icpx
module load gsl

MESH_SIZE=0.5
# I always try to pipe into cosma8's data directory, but only cosma5 is mounted
OUPUT_DIRECTORY=/cosma5/data/do009/frmh84/sbh-$MESH_SIZE
echo $OUPUT_DIRECTORY
rm -rf $OUPUT_DIRECTORY
mkdir $OUPUT_DIRECTORY
cd $OUPUT_DIRECTORY

$SLURM_SUBMIT_DIR/peano_sbh_$MESH_SIZE

