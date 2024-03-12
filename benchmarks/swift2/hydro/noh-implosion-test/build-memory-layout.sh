#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
#N_PART=2000
PLOT_DELTA=0.0

rm noh2D.*

set -e

for REALISATION in domain-decomposition.scattered.bucket-sort.no-optimisation domain-decomposition.scattered.bucket-sort.outer-guards \
                   domain-decomposition.scattered.multiscale-sort.no-optimisation domain-decomposition.scattered.multiscale-sort.outer-guards 
do
  echo build $REALISATION
  N_PART=1024
  PLOT_DELTA=0.0
  END_TIME=0.001
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION
  N_PART=120
  END_TIME=0.0005
  PLOT_DELTA=$END_TIME
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m asserts -o noh2D.$REALISATION-asserts
  #
  # See remarks on correctness checks in README.dox in the section "Validate correctness"
  #
  cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j
done

