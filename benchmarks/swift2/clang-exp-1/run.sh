#!/bin/sh

# Script to run experiment

# Parameters
END_TIME=1.0
CUT_OFF_RAD=0.15

# Run python script to generate the experiment
python3 planet-orbit.py -et=$END_TIME --cut-off $CUT_OFF_RAD --clean 

# set threads number
export OMP_NUM_THREADS=8

# run experiment
echo 'running experiment...'
./orbit-experiment > output.log

# generate Peano grid for Paraview
pvpython ../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Tidy up root folder
DIRNAME="snapshots_exp_r_""$CUT_OFF_RAD"
mkdir $DIRNAME
mv particles* grid* $DIRNAME


# @TODO: run plot script (e.g. energy conservation plot)
