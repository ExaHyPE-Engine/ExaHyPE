#!/bin/sh

# Script to run experiment (Noh 2D)

# set number of threads
export OMP_NUM_THREADS=28

# Parameters
TIMESTEP_SIZE=0.0001
END_TIME=0.0010
N_PART=100
CELL_SIZE=0.1
PLOT_DELTA=0

# Run python script to generate the experiment
python3 noh.py -j $OMP_NUM_THREADS -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -cs $CELL_SIZE -plot $PLOT_DELTA

# run experiment
echo 'running experiment...'
./noh2D

#likwid-perfctr -f -C 0 -g MEM            -o output_MEM.cvs            -O ./noh2D
#likwid-perfctr -f -C 0 -g DATA           -o output_DATA.cvs           -O ./noh2D
#likwid-perfctr -f -C 0 -g CACHES         -o output_CACHES.cvs         -O ./noh2D
#likwid-perfctr -f -C 0 -g L2CACHE        -o output_L2CACHE.cvs        -O ./noh2D
#likwid-perfctr -f -C 0 -g L3CACHE        -o output_L3CACHE.cvs        -O ./noh2D
#likwid-perfctr -f -C 0 -g CYCLE_ACTIVITY -o output_CYCLE_ACTIVITY.cvs -O ./noh2D

# generate Peano grid for Paraview (if needed)
#pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Save results into directory
#DIRNAME="output_np_"$N_PART"_et_"$END_TIME
#if [ -d $DIRNAME ]
#then
#  rm -r $DIRNAME
#fi
#
#mkdir $DIRNAME
#mv *.cvs $DIRNAME

# Save snapshots
#mkdir $DIRNAME"/snapshots/"
#mv particles* grid* $DIRNAME"/snapshots/"


# @TODO: run plot script
