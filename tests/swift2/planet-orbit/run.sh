#!/bin/sh

# --------------------------------------
# Script to run Peano SPH experiment
# Density calculaion test
# --------------------------------------

# exit on error
set -e

# set threads number
export OMP_NUM_THREADS=1

# Run python script to generate the experiment
# TODO: I'm not sure this is the intended way of running, which also would conserve energy and pass this test.
python3 planet-orbit.py --time-integrator leapfrog -et 5.0 -cs 0.3 -plot 0.01 --asserts

# run experiment
echo 'running experiment...'
./orbit-0.3 > output.txt

# generate Peano grid for Paraview (if needed)
# pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

python3 ./plots-time-integration-test.py

