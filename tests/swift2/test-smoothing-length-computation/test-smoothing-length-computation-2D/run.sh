#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=2

IC_FILENAME="test_sml_2D.hdf5"

# Parameters
N_PART=32
# if the cell size is too small, the initial search radius may be too small,
# and you won't find correct smoothing lengths. Instead, you'll get Warnings
# "h ~ h_max"
CELL_SIZE=0.3
H_TOLERANCE=1e-6


rand=$RANDOM
echo 'Generating initial conditions with seed' $rand
python3 makeIC.py -np $N_PART -s $rand

for neighbors in 8 15 20; do
    echo running for $neighbors neighbors
    # echo 'generate and compile code ...'
    python3 ./test_smoothing_length_computation_2D.py -cs $CELL_SIZE -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE --asserts
    echo 'running test ...'
    ./testSML | tee output-$neighbors.log
    echo 'comparing results ...'
    python3 ./compare_results.py -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE -d 2 | tee results-$neighbors.txt
    echo '---------------------'
done
