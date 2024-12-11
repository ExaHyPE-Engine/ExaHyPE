#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=1

IC_FILENAME="test_sml_multiscale_1D.hdf5"

# Parameters
N_PART=100
CELL_SIZE=0.5
H_TOLERANCE=1e-6


rand=$RANDOM
echo 'Generating initial conditions with seed' $rand
python3 makeIC-multiscale.py -np $N_PART -s $rand

for neighbors in 3 5 10; do
# for neighbors in 10; do
    echo running for $neighbors neighbors
    echo 'generate and compile code ...'
    python3 ./test_smoothing_length_computation_1D.py -cs $CELL_SIZE -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE --debug
    echo 'running test ...'
    ./testSML | tee output-multiscale-$neighbors.log
    echo 'comparing results ...'
    python3 ./compare_results.py -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE -d 1 | tee results-multiscale-$neighbors.log
    echo '---------------------'
done
