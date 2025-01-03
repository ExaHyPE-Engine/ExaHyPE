#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
#N_PART=2000
PLOT_DELTA=0.0

rm noh2D.*
rm profile-noh2D.*
rm asserts-noh2D.*
rm README-*.md

set -e


for TREES in 1 2 4 8 16 0
do
  for REALISATION in  domain-decomposition.scattered.bucket-sort.no-optimisation  \
                                task-graph.scattered.bucket-sort.no-optimisation \
                     multisweep-task-graph.scattered.bucket-sort.no-optimisation 
  do
    echo build $REALISATION
    N_PART=5000
    PLOT_DELTA=0.0
    END_TIME=0.1
    python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION --trees $TREES -v -m release -o noh2D.$REALISATION.$TREES-trees

    N_PART=1000
    PLOT_DELTA=0.0
    END_TIME=0.0005
    python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION --trees $TREES -v -m release -o profile-noh2D.$REALISATION.$TREES-trees

    N_PART=120
    PLOT_DELTA=0.001
    python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION --trees $TREES -v -m asserts -o asserts-noh2D.$REALISATION.$TREES-trees
  
    #
    # See remarks on correctness checks in README.dox in the section "Validate correctness"
    #
    # cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j
  done
done

