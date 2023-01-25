#!/bin/sh

# Script to run experiment (Noh 2D)

# set threads number
export OMP_NUM_THREADS=4

# Parameters
END_TIME=0.01
N_PART=33
CELL_SIZE=0.3

# Run python script to generate the experiment
python3 noh.py -cs $CELL_SIZE -et $END_TIME -np $N_PART

# run experiment
echo 'running experiment...'
./noh2D > output.txt

#likwid-perfctr -f -C 0 -g MEM            -o output_MEM.cvs            -O ./noh2D
#likwid-perfctr -f -C 0 -g DATA           -o output_DATA.cvs           -O ./noh2D
#likwid-perfctr -f -C 0 -g CACHES         -o output_CACHES.cvs         -O ./noh2D
#likwid-perfctr -f -C 0 -g L2CACHE        -o output_L2CACHE.cvs        -O ./noh2D
#likwid-perfctr -f -C 0 -g L3CACHE        -o output_L3CACHE.cvs        -O ./noh2D
#likwid-perfctr -f -C 0 -g CYCLE_ACTIVITY -o output_CYCLE_ACTIVITY.cvs -O ./noh2D

# generate Peano grid for Paraview (if needed)
#pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Save results into directory
DIRNAME="output"
if [ -d $DIRNAME ]
then
    rm -r $DIRNAME
fi
mkdir $DIRNAME
mv output.txt $DIRNAME

# Save file with parameters and metadata
mv Constants.h $DIRNAME

# Save snapshots
mkdir $DIRNAME"/snapshots/"
mv particles* grid* $DIRNAME"/snapshots/"

# Save other data, if any
if [ -e *.cvs ]
then
    mv *.cvs $DIRNAME
fi


# @TODO: run plot script if necessary
