#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
#N_PART=2000
PLOT_DELTA=0.0
<<<<<<< HEAD

rm noh2D.*

set -e

for REALISATION in domain-decomposition.scattered.bucket-sort.no-optimisation domain-decomposition.scattered.bucket-sort.outer-guards \
                   domain-decomposition.scattered.multiscale-sort.no-optimisation domain-decomposition.scattered.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.no-optimisation domain-decomposition.continuous-per-vertex.bucket-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-all domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.no-optimisation domain-decomposition.continuous-per-vertex.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-all domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.bucket-sort.no-optimisation domain-decomposition.global-continuous.bucket-sort.outer-guards \
                   domain-decomposition.global-continuous.bucket-sort.vectorise-all domain-decomposition.global-continuous.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.multiscale-sort.no-optimisation domain-decomposition.global-continuous.multiscale-sort.outer-guards \
                   domain-decomposition.global-continuous.multiscale-sort.vectorise-all domain-decomposition.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-tree.scattered.bucket-sort.no-optimisation task-tree.scattered.bucket-sort.outer-guards \
                   task-tree.scattered.multiscale-sort.no-optimisation task-tree.scattered.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.no-optimisation task-tree.continuous-per-vertex.bucket-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.vectorise-all task-tree.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-tree.continuous-per-vertex.multiscale-sort.no-optimisation task-tree.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.multiscale-sort.vectorise-all task-tree.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-tree.global-continuous.bucket-sort.no-optimisation task-tree.global-continuous.bucket-sort.outer-guards \
                   task-tree.global-continuous.bucket-sort.vectorise-all task-tree.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-tree.global-continuous.multiscale-sort.no-optimisation task-tree.global-continuous.multiscale-sort.outer-guards \
                   task-tree.global-continuous.multiscale-sort.vectorise-all task-tree.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-graph.scattered.bucket-sort.no-optimisation task-graph.scattered.bucket-sort.outer-guards \
                   task-graph.scattered.multiscale-sort.no-optimisation task-graph.scattered.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.no-optimisation task-graph.continuous-per-vertex.bucket-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.vectorise-all task-graph.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-graph.continuous-per-vertex.multiscale-sort.no-optimisation task-graph.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.multiscale-sort.vectorise-all task-graph.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-graph.global-continuous.bucket-sort.no-optimisation task-graph.global-continuous.bucket-sort.outer-guards \
                   task-graph.global-continuous.bucket-sort.vectorise-all task-graph.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-graph.global-continuous.multiscale-sort.no-optimisation task-graph.global-continuous.multiscale-sort.outer-guards \
                   task-graph.global-continuous.multiscale-sort.vectorise-all task-graph.global-continuous.multiscale-sort.vectorise-distance-checks
do
  echo build $REALISATION
  N_PART=1024
  PLOT_DELTA=0.0
  END_TIME=0.1
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION
  
  N_PART=120
  PLOT_DELTA=0.001
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION-test
  #
  # See remarks on correctness checks in README.dox in the section "Validate correctness"
  #
  cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j
  
  N_PART=120
  END_TIME=0.0005
  PLOT_DELTA=$END_TIME
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m asserts -o noh2D.$REALISATION-asserts
=======
TREES=8

rm noh2D.*

set -e

for REALISATION in domain-decomposition.scattered.bucket-sort.no-optimisation domain-decomposition.scattered.bucket-sort.outer-guards \
                   domain-decomposition.scattered.multiscale-sort.no-optimisation domain-decomposition.scattered.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.no-optimisation domain-decomposition.continuous-per-vertex.bucket-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-all domain-decomposition.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.no-optimisation domain-decomposition.continuous-per-vertex.multiscale-sort.outer-guards \
                   domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-all domain-decomposition.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.bucket-sort.no-optimisation domain-decomposition.global-continuous.bucket-sort.outer-guards \
                   domain-decomposition.global-continuous.bucket-sort.vectorise-all domain-decomposition.global-continuous.bucket-sort.vectorise-distance-checks \
                   domain-decomposition.global-continuous.multiscale-sort.no-optimisation domain-decomposition.global-continuous.multiscale-sort.outer-guards \
                   domain-decomposition.global-continuous.multiscale-sort.vectorise-all domain-decomposition.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-tree.scattered.bucket-sort.no-optimisation task-tree.scattered.bucket-sort.outer-guards \
                   task-tree.scattered.multiscale-sort.no-optimisation task-tree.scattered.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.no-optimisation task-tree.continuous-per-vertex.bucket-sort.outer-guards \
                   task-tree.continuous-per-vertex.bucket-sort.vectorise-all task-tree.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-tree.continuous-per-vertex.multiscale-sort.no-optimisation task-tree.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-tree.continuous-per-vertex.multiscale-sort.vectorise-all task-tree.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-tree.global-continuous.bucket-sort.no-optimisation task-tree.global-continuous.bucket-sort.outer-guards \
                   task-tree.global-continuous.bucket-sort.vectorise-all task-tree.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-tree.global-continuous.multiscale-sort.no-optimisation task-tree.global-continuous.multiscale-sort.outer-guards \
                   task-tree.global-continuous.multiscale-sort.vectorise-all task-tree.global-continuous.multiscale-sort.vectorise-distance-checks \
                   task-graph.scattered.bucket-sort.no-optimisation task-graph.scattered.bucket-sort.outer-guards \
                   task-graph.scattered.multiscale-sort.no-optimisation task-graph.scattered.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.no-optimisation task-graph.continuous-per-vertex.bucket-sort.outer-guards \
                   task-graph.continuous-per-vertex.bucket-sort.vectorise-all task-graph.continuous-per-vertex.bucket-sort.vectorise-distance-checks \
                   task-graph.continuous-per-vertex.multiscale-sort.no-optimisation task-graph.continuous-per-vertex.multiscale-sort.outer-guards \
                   task-graph.continuous-per-vertex.multiscale-sort.vectorise-all task-graph.continuous-per-vertex.multiscale-sort.vectorise-distance-checks \
                   task-graph.global-continuous.bucket-sort.no-optimisation task-graph.global-continuous.bucket-sort.outer-guards \
                   task-graph.global-continuous.bucket-sort.vectorise-all task-graph.global-continuous.bucket-sort.vectorise-distance-checks \
                   task-graph.global-continuous.multiscale-sort.no-optimisation task-graph.global-continuous.multiscale-sort.outer-guards \
                   task-graph.global-continuous.multiscale-sort.vectorise-all task-graph.global-continuous.multiscale-sort.vectorise-distance-checks
do
  clear
  echo build $REALISATION
  N_PART=1024
  PLOT_DELTA=0.0
  END_TIME=0.1
  python3 noh.py --trees $TREES -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION
  
  N_PART=120
  PLOT_DELTA=0.001
  python3 noh.py --trees $TREES -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION-test
  #
  # See remarks on correctness checks in README.dox in the section "Validate correctness"
  #
  cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j
  
  N_PART=120
  END_TIME=0.0005
  PLOT_DELTA=$END_TIME
  python3 noh.py --trees $TREES -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m asserts -o noh2D.$REALISATION-asserts
>>>>>>> refs/remotes/origin/p4
  #
  # See remarks on correctness checks in README.dox in the section "Validate correctness"
  #
  cp repositories/ModifiedGlobalState.cpp repositories/GlobalState.cpp; make -j
done

