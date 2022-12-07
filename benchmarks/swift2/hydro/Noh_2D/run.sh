#!/bin/sh

# Script to run experiment (Noh shockwave 1D)

# set threads number
export OMP_NUM_THREADS=1

# Parameters
END_TIME=0.2
N_PART=33

# Run python script to generate the experiment
python3 noh.py -et=$END_TIME -np $N_PART

# run experiment
echo 'running experiment...'
./noh1D

#likwid-perfctr -f -C 0 -g MEM            -o output_MEM.cvs            -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g DATA           -o output_DATA.cvs           -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g CACHES         -o output_CACHES.cvs         -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g L2CACHE        -o output_L2CACHE.cvs        -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g L3CACHE        -o output_L3CACHE.cvs        -O ./orbit-experiment
#likwid-perfctr -f -C 0 -g CYCLE_ACTIVITY -o output_CYCLE_ACTIVITY.cvs -O ./orbit-experiment

# generate Peano grid for Paraview (if needed)
#pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

# Save results into directory
DIRNAME="output"
if [ -d $DIRNAME ]
then
  rm -r $DIRNAME
fi

mkdir $DIRNAME
mv *.cvs Constants.h $DIRNAME

# Save snapshots
mkdir $DIRNAME"/snapshots/"
mv particles* grid* $DIRNAME"/snapshots/"


# @TODO: run plot script
