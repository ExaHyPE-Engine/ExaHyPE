#!/bin/sh

# exit on error
set -e

# Script to run experiment (Sedov-Taylor blast 2D)

# set threads number
export OMP_NUM_THREADS=4

# Parameters
N_PART=64
END_TIME=0.05
TIMESTEP_SIZE=5e-5
CELL_SIZE=0.3
PLOT_DELTA=0.01
CFL_FACTOR=0.2

# Run python script to generate the experiment
python3 sedov.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin=0.35 -csmax 0.35

# run experiment
echo 'running experiment...'
./sedov3D | tee output.txt

echo "Done."
