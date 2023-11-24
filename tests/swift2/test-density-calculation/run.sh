#!/bin/sh

# --------------------------------------
# Script to run Peano SPH experiment
# Density calculaion test
# --------------------------------------

# set threads number
export OMP_NUM_THREADS=1

# Parameters
END_TIME=0.01
N_PART=100
CELL_SIZE=0.3

# Run python script to generate the experiment
python3 test-density-calculation_1D.py -et $END_TIME -np $N_PART -cs $CELL_SIZE

# run experiment
echo 'running experiment...'
./test_density > output.txt

# generate Peano grid for Paraview (if needed)
pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file


# @TODO: run plot script
