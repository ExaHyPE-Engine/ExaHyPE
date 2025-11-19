#!/bin/sh

set -e

# Script to run experiment (Noh 2D)

# set threads number
export OMP_NUM_THREADS=4

# Parameters
N_PART=32
END_TIME=0.2
TIMESTEP_SIZE=0.0001
CFL_FACTOR=0.2
PLOT_DELTA=0.01

# Run python script to generate the experiment
echo 'generate and compile code ...'
echo python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin=0.67 -csmax 0.67 --debug
python3 noh.py -np $N_PART -et $END_TIME -dt $TIMESTEP_SIZE -cfl $CFL_FACTOR -plot $PLOT_DELTA -csmin=0.67 -csmax 0.67 --debug

# run experiment
echo 'running experiment...'
./noh2D | tee output.txt
echo 'experiment run finished.'


echo 'Done. Bye!'
