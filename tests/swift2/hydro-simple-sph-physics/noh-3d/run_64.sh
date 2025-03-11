#!/bin/sh

set -e

# Script to run experiment (Noh 3D)

# set threads number
export OMP_NUM_THREADS=4

# Parameters
N_PART=64
END_TIME=0.6
TIMESTEP_SIZE=0.0001
CFL_FACTOR=0.2
PLOT_DELTA=0.05
CELL_SIZE_MIN=0.35
CELL_SIZE_MAX=0.35

# Run python script to generate the experiment
echo 'generate and compile code ...'
echo python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE
python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin $CELL_SIZE_MIN -csmax $CELL_SIZE_MAX

# run experiment
echo 'running experiment...'
./noh3D | tee output.txt
echo 'experiment run finished.'

# echo 'post-processing results...'

# generate Peano grid for Paraview (if needed)
# pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Save results into directory
# DIRNAME="output"
# if [ -d $DIRNAME ]
# then
#     rm -r $DIRNAME
# fi
# mkdir $DIRNAME
# mv output.txt $DIRNAME
#
# Save file with parameters and metadata
# less Constants.h > Constants.txt && mv Constants.txt $DIRNAME

# Save snapshots
# mkdir $DIRNAME"/snapshots/"
# mv particles* grid* $DIRNAME"/snapshots/"

# Save other data, if any
# if [ -e *.cvs ]
# then
#     mv *.cvs $DIRNAME
# fi
#
echo 'Done. Bye!'
