#!/bin/sh

export PYTHONPATH=$PYTHONPATH:../../../../python

# Script to run experiment (Noh 2D)

# Parameters
TIMESTEP_SIZE=0.0001
#N_PART=2000
PLOT_DELTA=0.0

rm noh2D.*

set -e

for REALISATION in domain-decomposition.scattered.sort-on-the-fly.no-optimisation domain-decomposition.scattered.sort-on-the-fly.outer-guards \
                   domain-decomposition.scattered.insert-sorts.no-optimisation domain-decomposition.scattered.insert-sorts.outer-guards \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.no-optimisation domain-decomposition.continuous-per-vertex.sort-on-the-fly.outer-guards \
                   domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-all domain-decomposition.continuous-per-vertex.sort-on-the-fly.vectorise-distance-checks \
                   domain-decomposition.continuous-per-vertex.insert-sorts.no-optimisation domain-decomposition.continuous-per-vertex.insert-sorts.outer-guards \
                   domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-all domain-decomposition.continuous-per-vertex.insert-sorts.vectorise-distance-checks \
                   domain-decomposition.global-continuous.sort-on-the-fly.no-optimisation domain-decomposition.global-continuous.sort-on-the-fly.outer-guards \
                   domain-decomposition.global-continuous.sort-on-the-fly.vectorise-all domain-decomposition.global-continuous.sort-on-the-fly.ovectorise-distance-checks \
                   domain-decomposition.global-continuous.insert-sorts.no-optimisation domain-decomposition.global-continuous.insert-sorts.outer-guards \
                   domain-decomposition.global-continuous.insert-sorts.vectorise-all domain-decomposition.global-continuous.insert-sorts.vectorise-distance-checks
do
  N_PART=240
  PLOT_DELTA=0.0
  END_TIME=0.001
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m release -o noh2D.$REALISATION
  N_PART=120
  PLOT_DELTA=$END_TIME
  END_TIME=0.0005
  python3 noh.py -dt $TIMESTEP_SIZE -et $END_TIME -np $N_PART -plot $PLOT_DELTA -rea $REALISATION -v -m asserts -o noh2D.$REALISATION-asserts
done

