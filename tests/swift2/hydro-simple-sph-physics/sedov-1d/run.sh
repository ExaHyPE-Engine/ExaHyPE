#!/bin/sh

# exit on error
set -e

# Script to run experiment (Sedov-Taylor blast 1D)

# set threads number
export OMP_NUM_THREADS=1

# Parameters
N_PART=201
END_TIME=0.05
TIMESTEP_SIZE=1e-4
CELL_SIZE=0.3
PLOT_DELTA=0.01
CFL_FACTOR=0.2

# Run python script to generate the experiment
# python3 sedov.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin=0.04 -csmax 0.04
python3 sedov.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin=0.04 -csmax 0.04

# run experiment
echo 'running experiment...'
./sedov1D | tee output.txt

echo "Done."
