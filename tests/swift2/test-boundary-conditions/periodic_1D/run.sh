#!/bin/sh

# exit on error
set -e

# set threads number
export OMP_NUM_THREADS=1

END_TIME=1.0
# END_TIME=0.5

# Run python script to generate the experiment
python3 test_periodic_BC-1D.py -et $END_TIME -dt 0.001 -cs 0.1 -plot 0.01 --debug

# run experiment
echo 'running experiment...'
./periodicBCTest1D 2>&1 | tee output.txt

python3 compare_particle_positions.py -et $END_TIME -d 1

# generate Peano grid for Paraview (if needed)
# pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

