#!/bin/sh

# exit on error
set -e

# Script to run experiment (Sedov-Taylor blast 1D)

# set threads number
export OMP_NUM_THREADS=1

# File with initial conditions
#IC_FILENAME="sedov.hdf5"

# Parameters
END_TIME=0.01
TIME_STEP_SIZE=1e-4
N_PART=65
CELL_SIZE=0.3

# Generate initial condition file
if [ ! -e $IC_FILENAME ]
then
  echo 'Generating initial conditions...'
  python3 makeIC.py
fi

# Run python script to generate the experiment
python3 sedov.py -dt $TIME_STEP_SIZE -et $END_TIME -np $N_PART -cs $CELL_SIZE 

# run experiment
echo 'running experiment...'
./sedov1D | tee output.txt

echo "Done."
