#!/bin/sh

# Script to run experiment (Noh 2D)

# set number of threads
export OMP_NUM_THREADS=32

# Parameters
MANTISSA_SIZE=10
TIMESTEP_SIZE=0.0001
END_TIME=0.1
N_PART=400
CELL_SIZE=0.1

# Run python script to generate the experiment
python3 noh.py -j $OMP_NUM_THREADS -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -cs $CELL_SIZE -ms $MANTISSA_SIZE

# run experiment
echo 'running experiment...'
./noh2D > p4_output.txt

