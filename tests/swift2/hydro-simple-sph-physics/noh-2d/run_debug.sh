#!/bin/sh

set -e

# Script to run experiment (Noh 2D)

# set threads number
export OMP_NUM_THREADS=1

# Parameters
N_PART=30
END_TIME=0.01
TIMESTEP_SIZE=0.0001
CFL_FACTOR=0.2
PLOT_DELTA=0.01
CELL_SIZE=0.3

# Run python script to generate the experiment
echo 'generate and compile code ...'
echo python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE
python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE  --debug

# run experiment
echo 'running experiment...'
./noh2D | tee output.txt
echo 'experiment run finished.'

