#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=4

IC_FILENAME="test_sml_1D.hdf5"

# Parameters
N_PART=900          # fixed by IC file
CELL_SIZE=0.1


# Generate initial condition file
if [ ! -e $IC_FILENAME ]
then
    rand=$RANDOM
    echo 'Generating initial conditions with seed' $rand
    python3 makeIC.py -np $N_PART -s $rand
fi

# echo python3 sod.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE

# for neighbors in 1 2 5 10; do
for neighbors in 6; do
    # Run python script to generate the exper# iment
    echo 'generate and compile code ...'
    python3 ./test_smoothing_length_computation_1D.py -cs $CELL_SIZE -ic $IC_FILENAME -n $neighbors
    echo 'running test ...'
    ./testSML
    echo 'comparing results ...'
    python3 ./compare_results.py -ic $IC_FILENAME -n $neighbors
done
