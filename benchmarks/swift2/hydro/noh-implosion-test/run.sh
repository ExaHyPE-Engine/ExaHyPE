#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
END_TIME=0.0001
#N_PART=2000
N_PART=200
PLOT_DELTA=0

# Run python script to generate the experiment
python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -opt continuous-per-vertex-vectorise -v

