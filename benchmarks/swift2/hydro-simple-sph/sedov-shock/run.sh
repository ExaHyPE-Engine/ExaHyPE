#!/bin/sh

# Script to run experiment (Sedov-Taylor blast 1D)

# set threads number
export OMP_NUM_THREADS=1

# File with initial conditions
IC_FILENAME="sedov.hdf5"

# Parameters
END_TIME=0.05
TIME_STEP_SIZE=1e-4
#N_PART=128
N_PART=100
CELL_SIZE=0.5

# Generate initial condition file
if [ ! -e $IC_FILENAME ]
then
  echo 'Generating initial conditions...'
  python3 makeIC.py
fi

# Run python script to generate the experiment
python3 sedov.py -dt $TIME_STEP_SIZE -et $END_TIME -np $N_PART -cs $CELL_SIZE 

# run experiment
echo 'running experiment...'
./sedov1D > output.txt

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
mv  Constants.h $DIRNAME

# Save snapshots
mkdir $DIRNAME"/snapshots/"
mv particles* grid* $DIRNAME"/snapshots/"

# Save other outputs, if any
if [ -e *.cvs ]; then
  mv *.cvs $DIRNAME
fi

echo "Done."

# @TODO: run plot script
