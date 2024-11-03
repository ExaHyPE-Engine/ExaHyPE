#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=4

IC_FILENAME="test_sml_3D.hdf5"

# Parameters
N_PART=20
# if the cell size is too small, the initial search radius may be too small,
# and you won't find correct smoothing lengths. Instead, you'll get Warnings
# "h ~ h_max"
CELL_SIZE=0.3
H_TOLERANCE=1e-6


# rand=$RANDOM
rand=22221
echo 'Generating initial conditions with seed' $rand
python3 makeIC-multiscale.py -np $N_PART -s $rand

for neighbors in 30 48 64; do
    echo running for $neighbors neighbors
    # echo 'generate and compile code ...'
    python3 ./test_smoothing_length_computation_3D.py -cs $CELL_SIZE -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE --asserts
    echo 'running test ...'
    ./testSML | tee output-$neighbors.log
    echo 'comparing results ...'
    python3 ./compare_results.py -ic $IC_FILENAME -n $neighbors -ht $H_TOLERANCE -d 3 | tee results-$neighbors.txt
    echo '---------------------'
done
