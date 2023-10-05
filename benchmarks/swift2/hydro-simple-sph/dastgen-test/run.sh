#!/bin/sh

# exit on error
set -e

# Script to run experiment (dastgen-test)
#
# set threads number
export OMP_NUM_THREADS=1

# Run python script to generate the experiment
python3 dastgen-test.py

# run experiment
echo 'running experiment...'
./dastgen_test | tee output.txt

echo "Done."
