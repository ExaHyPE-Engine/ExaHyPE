#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=4

IC_FILENAME="test_sml_1D.hdf5"

# Parameters
N_PART=1000          # fixed by IC file
CELL_SIZE=0.1
H_TOLERANCE=1e-6


rand=$RANDOM
echo 'Generating initial conditions with seed' $rand
python3 makeIC.py -np $N_PART -s $rand

for neighbors in 3 5 10; do
    echo running for $neighbors neighbors
    echo 'generate and compile code ...'
    python3 ./test_smoothing_length_computation_1D.py -cs $CELL_SIZE -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE > /dev/null
    echo 'running test ...'
    ./testSML > /dev/null
    echo 'comparing results ...'
    python3 ./compare_results.py -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE -d 1
    echo '---------------------'
done
