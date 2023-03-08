#!/bin/sh

# Script to run experiment

# Parameters
END_TIME=0.5 
CELL_SIZE=0.4 # level 0
PLOT_DELTA=0.1

# Run python script to generate the experiment
python3 planet-orbit.py -integrator leapfrog -et=$END_TIME -cs=$CELL_SIZE -plot=$PLOT_DELTA

# set threads number
export OMP_NUM_THREADS=1

# run experiment
echo 'running experiment...'

./orbit-experiment

#likwid-perfctr -f -C 0 -g MEM            -o output_MEM.cvs            -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g DATA           -o output_DATA.cvs           -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g CACHES         -o output_CACHES.cvs         -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g L2CACHE        -o output_L2CACHE.cvs        -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g L3CACHE        -o output_L3CACHE.cvs        -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g CYCLE_ACTIVITY -o output_CYCLE_ACTIVITY.cvs -O ./orbit-experiment

# generate Peano grid for Paraview
#pvpython ../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Tidy up root folder and back up results
DIRNAME="output_cs_""$CELL_SIZE"
if [ -d $DIRNAME ]
then
  rm -r $DIRNAME
fi

mkdir $DIRNAME
mv *.cvs $DIRNAME
mv Constants.h $DIRNAME
mkdir $DIRNAME"/snapshots/"
mv particles* grid* $DIRNAME"/snapshots/"


