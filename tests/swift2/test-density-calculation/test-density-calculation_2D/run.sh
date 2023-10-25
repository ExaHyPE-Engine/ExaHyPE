#!/bin/sh

# --------------------------------------
# Script to run Peano SPH experiment
# Density calculaion test
# --------------------------------------

# set threads number
export OMP_NUM_THREADS=1

# Parameters
DIMENSIONS=2
END_TIME=0.01
N_PART=32
CELL_SIZE=0.3

# Run python script to generate the experiment
python3 test-density-calculation.py -et $END_TIME -np $N_PART -cs $CELL_SIZE -d $DIMENSIONS

# run experiment
echo 'running experiment...'
./test_density > output.txt

# generate Peano grid for Paraview (if needed)
pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file


# @TODO: run plot script
