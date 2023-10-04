#!/bin/sh

# exit on error
set -e

# Script to run experiment (Sedov-Taylor blast 1D)

# set threads number
export OMP_NUM_THREADS=1

# Parameters
END_TIME=1
TIME_STEP_SIZE=1
N_PART=65
CELL_SIZE=0.3

# Run python script to generate the experiment
# python3 dastgen-test.py -dt $TIME_STEP_SIZE -et $END_TIME -cs $CELL_SIZE
python3 dastgen-test.py -cs $CELL_SIZE 

# run experiment
echo 'running experiment...'
./dastgen_test | tee output.txt

echo "Done."
