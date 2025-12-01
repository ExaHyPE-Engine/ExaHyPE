#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=4

IC_FILENAME="sodShock.hdf5"

# Parameters
N_PART=900          # fixed by IC file
# END_TIME=9.765625e-03
END_TIME=0.2      # original setup
TIMESTEP_SIZE=9.765625e-05 # copied from SWIFT output.
CFL_FACTOR=0.1
PLOT_DELTA=0.01
CELL_SIZE_MAX=0.6 # note that boxsize here is 2. This should give us cells of size 0.444
CELL_SIZE_MIN=0.3 # note that boxsize here is 2. This should give us cells of size 0.222

# Generate initial condition file
if [ ! -e $IC_FILENAME ]
then
    echo 'Generating initial conditions...'
    python3 makeIC.py
fi

# Run python script to generate the experiment
echo 'generate and compile code ...'
echo python3 sod.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE
python3 sod.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin $CELL_SIZE_MIN -csmax $CELL_SIZE_MIN --debug

# run experiment
echo 'running experiment...'
./sod1D | tee output.txt
echo 'experiment run finished.'

