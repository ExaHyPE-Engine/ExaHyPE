#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
#N_PART=2000
PLOT_DELTA=0.0

rm noh2D.*
rm profile-noh2D.*
rm README-*.md

set -e

for REALISATION in domain-decomposition.scattered.bucket-sort.no-optimisation domain-decomposition.continuous-per-vertex.bucket-sort.no-optimisation domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-distance-checks-preamble domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-all
do
  echo build $REALISATION
  N_PART=5000
  PLOT_DELTA=0.0
  END_TIME=0.1
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION

  echo build $REALISATION
  N_PART=2000
  PLOT_DELTA=0.0
  END_TIME=0.0005
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o profile-noh2D.$REALISATION

  # N_PART=120
  # PLOT_DELTA=0.001
  # python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION-test
  # #
  # # See remarks on correctness checks in README.dox in the section "Validate correctness"
  # #
  # cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j

done

