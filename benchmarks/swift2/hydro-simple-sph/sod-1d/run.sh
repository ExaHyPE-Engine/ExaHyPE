#!/bin/bash

# exit on error
set -e
set -o pipefail

# set threads number
export OMP_NUM_THREADS=4

IC_FILENAME="sodShock.hdf5"

# Parameters
N_PART=900          # fixed by IC file
END_TIME=0.025
# TIMESTEP_SIZE=2e-4
# TIMESTEP_SIZE=1.953125e-04
TIMESTEP_SIZE=9.765625e-05
CFL_FACTOR=0.1
# PLOT_DELTA=0.01
# PLOT_DELTA=1.953125e-04
PLOT_DELTA=9.765625e-05
# TODO: this is temporary until I fix smoothing length computations so the test doesn't crash.
CELL_SIZE=0.1
# CELL_SIZE=0.67
#
# Generate initial condition file
if [ ! -e $IC_FILENAME ]
then
    echo 'Generating initial conditions...'
    python3 makeIC.py
fi

# Run python script to generate the experiment
echo 'generate and compile code ...'
echo python3 sod.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE
python3 sod.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -cs $CELL_SIZE --asserts
# exit

# run experiment
echo 'running experiment...'
./sod1D | tee output.txt
echo 'experiment run finished.'

echo 'post-processing results...'

# generate Peano grid for Paraview (if needed)
pvpython ../../../../python/peano4/visualisation/render.py grid.peano-patch-file

# # Save results into directory
# DIRNAME="output"
# if [ -d $DIRNAME ]
# then
#     rm -r $DIRNAME
# fi
# mkdir $DIRNAME
# mv output.txt $DIRNAME
#
# # Save file with parameters and metadata
# less Constants.h > Constants.txt && mv Constants.txt $DIRNAME
#
# # Save snapshots
# mkdir $DIRNAME"/snapshots/"
# mv particles* grid* $DIRNAME"/snapshots/"
#
# # Save other data, if any
# if [ -e *.csv ]
# then
#     mv *.csv $DIRNAME
# fi

# plot the solution
# python plotSolution.py 0.1 # not ready yet
