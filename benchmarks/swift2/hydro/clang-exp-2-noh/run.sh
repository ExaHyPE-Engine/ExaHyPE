#!/bin/sh

# Script to run experiment (Noh 2D)

# set number of threads
export OMP_NUM_THREADS=128

# Parameters
TIMESTEP_SIZE=0.0001
END_TIME=0.01
N_PART=400
CELL_SIZE=0.1
PLOT_DELTA=0

# Run python script to generate the experiment
python3 noh.py -j $OMP_NUM_THREADS -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -cs $CELL_SIZE -plot $PLOT_DELTA
